/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include "s3_get_object_action.h"
#include "s3_clovis_layout.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_option.h"

S3GetObjectAction::S3GetObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3ClovisReaderFactory> clovis_s3_factory)
    : S3Action(req),
      total_blocks_in_object(0),
      blocks_already_read(0),
      data_sent_to_client(0),
      read_object_reply_started(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  object_list_oid = {0ULL, 0ULL};

  s3_log(S3_LOG_INFO, request_id, "S3 API: Get Object. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  if (object_meta_factory) {
    object_metadata_factory = object_meta_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }

  if (clovis_s3_factory) {
    clovis_reader_factory = clovis_s3_factory;
  } else {
    clovis_reader_factory = std::make_shared<S3ClovisReaderFactory>();
  }

  setup_steps();
}

void S3GetObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3GetObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3GetObjectAction::fetch_object_info, this));
  add_task(std::bind(&S3GetObjectAction::read_object, this));
  add_task(std::bind(&S3GetObjectAction::send_response_to_s3_client, this));
  // ...
}

void S3GetObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Fetching bucket metadata\n");

  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(std::bind(&S3GetObjectAction::next, this),
                        std::bind(&S3GetObjectAction::next, this));
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "get_object_action_fetch_bucket_info_shutdown_fail");
}

void S3GetObjectAction::fetch_object_info() {
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Found bucket metadata\n");
    object_list_oid = bucket_metadata->get_object_list_index_oid();
    if (object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL) {
      // There is no object list index, hence object doesn't exist
      s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
      set_s3_error("NoSuchKey");
      send_response_to_s3_client();
    } else {
      object_metadata = object_metadata_factory->create_object_metadata_obj(
          request, object_list_oid);

      object_metadata->load(std::bind(&S3GetObjectAction::next, this),
                            std::bind(&S3GetObjectAction::next, this));
    }
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
}

void S3GetObjectAction::read_object() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    request->set_out_header_value("Last-Modified",
                                  object_metadata->get_last_modified_gmt());
    request->set_out_header_value("ETag", object_metadata->get_md5());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Length",
                                  object_metadata->get_content_length_str());
    for (auto it : object_metadata->get_user_attributes()) {
      request->set_out_header_value(it.first, it.second);
    }
    request->send_reply_start(S3HttpSuccess200);
    read_object_reply_started = true;

    if (object_metadata->get_content_length() == 0) {
      s3_log(S3_LOG_DEBUG, request_id, "Found object of size %zu\n",
             object_metadata->get_content_length());
      send_response_to_s3_client();
    } else {
      s3_log(S3_LOG_DEBUG, request_id, "Reading object of size %zu\n",
             object_metadata->get_content_length());

      size_t clovis_unit_size =
          S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(
              object_metadata->get_layout_id());
      s3_log(S3_LOG_DEBUG, request_id,
             "clovis_unit_size = %zu for layout_id = %d\n", clovis_unit_size,
             object_metadata->get_layout_id());
      /* Count Data blocks from data size */
      total_blocks_in_object =
          (object_metadata->get_content_length() + (clovis_unit_size - 1)) /
          clovis_unit_size;

      clovis_reader = clovis_reader_factory->create_clovis_reader(
          request, object_metadata->get_oid(),
          object_metadata->get_layout_id());
      read_object_data();
    }
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
    set_s3_error("NoSuchKey");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Object metadata fetch failed\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetObjectAction::read_object_data() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }

  size_t max_blocks_in_one_read_op =
      S3Option::get_instance()->get_clovis_units_per_request();
  size_t blocks_to_read = 0;

  if (blocks_already_read != total_blocks_in_object) {
    if ((total_blocks_in_object - blocks_already_read) >
        max_blocks_in_one_read_op) {
      blocks_to_read = max_blocks_in_one_read_op;
    } else {
      blocks_to_read = total_blocks_in_object - blocks_already_read;
    }

    if (blocks_to_read > 0) {
      bool op_launched = clovis_reader->read_object_data(
          blocks_to_read,
          std::bind(&S3GetObjectAction::send_data_to_client, this),
          std::bind(&S3GetObjectAction::read_object_data_failed, this));
      if (!op_launched) {
        set_s3_error("InternalError");
        send_response_to_s3_client();
      }
    } else {
      send_response_to_s3_client();
    }
  } else {
    // We are done Reading
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetObjectAction::send_data_to_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Earlier data_sent_to_client = %zu bytes.\n",
         data_sent_to_client);

  char* data = NULL;
  size_t length = 0;
  size_t obj_size = object_metadata->get_content_length();

  length = clovis_reader->get_first_block(&data);
  while (length > 0) {
    blocks_already_read++;
    if ((data_sent_to_client + length) >= obj_size) {
      length = obj_size - data_sent_to_client;
    }
    data_sent_to_client += length;
    s3_log(S3_LOG_DEBUG, request_id, "Sending %zu bytes to client.\n", length);
    request->send_reply_body(data, length);
    length = clovis_reader->get_next_block(&data);
  }
  if (data_sent_to_client != obj_size) {
    read_object_data();
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetObjectAction::read_object_data_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Failed to read object data from clovis\n");
  set_s3_error("InternalError");
  send_response_to_s3_client();
}

void S3GetObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (reject_if_shutting_down()) {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      // Send response with 'Service Unavailable' code.
      s3_log(S3_LOG_DEBUG, request_id,
             "sending 'Service Unavailable' response...\n");
      S3Error error("ServiceUnavailable", request->get_request_id(),
                    request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->set_out_header_value("Content-Length",
                                    std::to_string(response_xml.length()));
      request->set_out_header_value("Retry-After", "1");

      request->send_response(error.get_http_status_code(), response_xml);
    }
  } else if (is_error_state() && !get_s3_error_code().empty()) {
    // Invalid Bucket Name
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata &&
             ((object_metadata->get_content_length() == 0) ||
              (clovis_reader->get_state() == S3ClovisReaderOpState::success))) {
    request->send_reply_end();
  } else {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      S3Error error("InternalError", request->get_request_id(),
                    request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->set_out_header_value("Content-Length",
                                    std::to_string(response_xml.length()));
      request->set_out_header_value("Retry-After", "1");

      request->send_response(error.get_http_status_code(), response_xml);
    }
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}

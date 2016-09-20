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
#include "s3_option.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetObjectAction::S3GetObjectAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req),
      total_blocks_in_object(0),
      blocks_already_read(0),
      data_sent_to_client(0),
      read_object_reply_started(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  setup_steps();
}

void S3GetObjectAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind( &S3GetObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3GetObjectAction::fetch_object_info, this ));
  add_task(std::bind( &S3GetObjectAction::read_object, this ));
  add_task(std::bind( &S3GetObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Fetching bucket metadata\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3GetObjectAction::next, this), std::bind( &S3GetObjectAction::next, this));
}

void S3GetObjectAction::fetch_object_info() {
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, "Found bucket metadata\n");
    object_metadata = std::make_shared<S3ObjectMetadata>(request);

    object_metadata->load(std::bind( &S3GetObjectAction::next, this), std::bind( &S3GetObjectAction::next, this));
  } else {
    s3_log(S3_LOG_DEBUG, "Bucket not found\n");
    send_response_to_s3_client();
  }
}

void S3GetObjectAction::read_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    request->set_out_header_value("Last-Modified", object_metadata->get_last_modified_gmt());
    request->set_out_header_value("ETag", object_metadata->get_md5());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Length", object_metadata->get_content_length_str());
    for ( auto it: object_metadata->get_user_attributes() ) {
      request->set_out_header_value(it.first, it.second);
    }
    request->send_reply_start(S3HttpSuccess200);
    read_object_reply_started = true;

    if (object_metadata->get_content_length() == 0) {
      s3_log(S3_LOG_DEBUG, "Found object of size %zu\n", object_metadata->get_content_length());
      send_response_to_s3_client();
    } else {
      s3_log(S3_LOG_DEBUG, "Reading object of size %zu\n", object_metadata->get_content_length());

      size_t clovis_block_size = S3Option::get_instance()->get_clovis_block_size();
      /* Count Data blocks from data size */
      total_blocks_in_object = (object_metadata->get_content_length() + (clovis_block_size - 1)) / clovis_block_size;

      clovis_reader = std::make_shared<S3ClovisReader>(request, std::make_shared<ConcreteClovisAPI>(), object_metadata->get_oid());
      read_object_data();
    }
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3GetObjectAction::read_object_data() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }

  size_t max_blocks_in_one_read_op = S3Option::get_instance()->get_clovis_read_payload_size()/S3Option::get_instance()->get_clovis_block_size();
  size_t blocks_to_read = 0;

  if (blocks_already_read != total_blocks_in_object) {
    if ((total_blocks_in_object - blocks_already_read) > max_blocks_in_one_read_op) {
      blocks_to_read = max_blocks_in_one_read_op;
    } else {
      blocks_to_read = total_blocks_in_object - blocks_already_read;
    }

    if (blocks_to_read > 0) {
      clovis_reader->read_object_data(blocks_to_read, std::bind( &S3GetObjectAction::send_data_to_client, this), std::bind( &S3GetObjectAction::read_object_data_failed, this));
    } else {
      send_response_to_s3_client();
    }
  } else {
    // We are done Reading
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3GetObjectAction::send_data_to_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }

  char *data = NULL;
  size_t length = 0;
  size_t obj_size = object_metadata->get_content_length();

  length = clovis_reader->get_first_block(&data);
  while(length > 0)
  {
    blocks_already_read++;
    if ((data_sent_to_client + length) >= obj_size) {
      length = obj_size - data_sent_to_client;
    }

    data_sent_to_client += length;
    request->send_reply_body(data, length);
    length = clovis_reader->get_next_block(&data);
  }
  if (data_sent_to_client != obj_size) {
    read_object_data();
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3GetObjectAction::read_object_data_failed() {
  s3_log(S3_LOG_DEBUG, "Failed to read object data from clovis\n");
  send_response_to_s3_client();
}


void S3GetObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (reject_if_shutting_down()) {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      // Send response with 'Service Unavailable' code.
      s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
      request->set_out_header_value("Retry-After", "1");
      request->send_response(S3HttpFailed503);
    }
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("InvalidBucketName", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    S3Error error("NoSuchKey", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if ((object_metadata->get_content_length() == 0) || (clovis_reader->get_state() == S3ClovisReaderOpState::success)) {
    request->send_reply_end();
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

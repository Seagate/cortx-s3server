/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Siddhivinayak Shanbhag <siddhivinayak.shanbhag@seagate.com>
 * Original creation date: 24-July-2020
 */

#include "mero_get_object_action.h"
#include "s3_error_codes.h"
#include "s3_common_utilities.h"
#include "s3_m0_uint128_helper.h"

MeroGetObjectAction::MeroGetObjectAction(
    std::shared_ptr<MeroRequestObject> req,
    std::shared_ptr<S3ClovisReaderFactory> reader_factory)
    : MeroAction(std::move(req)) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");

  if (reader_factory) {
    clovis_reader_factory = std::move(reader_factory);
  } else {
    clovis_reader_factory = std::make_shared<S3ClovisReaderFactory>();
  }

  setup_steps();
}

void MeroGetObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(MeroGetObjectAction::validate_request, this);
  ACTION_TASK_ADD(MeroGetObjectAction::read_object, this);
  ACTION_TASK_ADD(MeroGetObjectAction::send_response_to_s3_client, this);
  // ...
}

void MeroGetObjectAction::validate_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  oid = S3M0Uint128Helper::to_m0_uint128(request->get_object_oid_lo(),
                                         request->get_object_oid_hi());
  // invalid oid
  if (!oid.u_hi && !oid.u_lo) {
    s3_log(S3_LOG_ERROR, request_id, "Invalid object oid\n");
    set_s3_error("BadRequest");
    send_response_to_s3_client();
  } else {
    std::string object_layout_id = request->get_query_string_value("layout-id");
    if (!S3CommonUtilities::stoi(object_layout_id, layout_id)) {
      s3_log(S3_LOG_ERROR, request_id, "Invalid object layout-id: %s\n",
             object_layout_id.c_str());
      set_s3_error("BadRequest");
      send_response_to_s3_client();
    } else {
      next();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroGetObjectAction::set_total_blocks_to_read_from_object() {
  // to read complete object, total number blocks to read is equal to total
  // number of blocks
  if ((first_byte_offset_to_read == 0) &&
      (last_byte_offset_to_read == (content_length - 1))) {
    total_blocks_to_read = total_blocks_in_object;
  } else {
    // object read for valid range
    size_t clovis_unit_size =
        S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(layout_id);
    // get block of first_byte_offset_to_read
    size_t first_byte_offset_block =
        (first_byte_offset_to_read + clovis_unit_size) / clovis_unit_size;
    // get block of last_byte_offset_to_read
    size_t last_byte_offset_block =
        (last_byte_offset_to_read + clovis_unit_size) / clovis_unit_size;
    // get total number blocks to read for a given valid range
    total_blocks_to_read = last_byte_offset_block - first_byte_offset_block + 1;
  }
}

void MeroGetObjectAction::read_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // get total number of blocks to read from an object
  set_total_blocks_to_read_from_object();
  clovis_reader =
      clovis_reader_factory->create_clovis_reader(request, oid, layout_id);
  // get the block,in which first_byte_offset_to_read is present
  // and initilaize the last index with starting offset the block
  size_t block_start_offset =
      first_byte_offset_to_read -
      (first_byte_offset_to_read %
       S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(layout_id));
  clovis_reader->set_last_index(block_start_offset);
  read_object_data();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroGetObjectAction::read_object_data() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }

  size_t max_blocks_in_one_read_op =
      S3Option::get_instance()->get_clovis_units_per_request();
  size_t blocks_to_read = 0;

  s3_log(S3_LOG_DEBUG, request_id, "max_blocks_in_one_read_op: (%zu)\n",
         max_blocks_in_one_read_op);
  s3_log(S3_LOG_DEBUG, request_id, "blocks_already_read: (%zu)\n",
         blocks_already_read);
  s3_log(S3_LOG_DEBUG, request_id, "total_blocks_to_read: (%zu)\n",
         total_blocks_to_read);
  if (blocks_already_read != total_blocks_to_read) {
    if ((total_blocks_to_read - blocks_already_read) >
        max_blocks_in_one_read_op) {
      blocks_to_read = max_blocks_in_one_read_op;
    } else {
      blocks_to_read = total_blocks_to_read - blocks_already_read;
    }
    s3_log(S3_LOG_DEBUG, request_id, "blocks_to_read: (%zu)\n", blocks_to_read);

    if (blocks_to_read > 0) {
      bool op_launched = clovis_reader->read_object_data(
          blocks_to_read,
          std::bind(&MeroGetObjectAction::send_data_to_client, this),
          std::bind(&MeroGetObjectAction::read_object_data_failed, this));
      if (!op_launched) {
        if (clovis_reader->get_state() ==
            S3ClovisReaderOpState::failed_to_launch) {
          set_s3_error("ServiceUnavailable");
          s3_log(S3_LOG_ERROR, request_id,
                 "read_object_data called due to clovis_entity_open failure\n");
        } else {
          set_s3_error("InternalError");
        }
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

void MeroGetObjectAction::send_data_to_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  log_timed_counter(get_timed_counter, "outgoing_object_data_blocks");

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (!read_object_reply_started) {
    // s3_timer.start();

    // AWS add explicit quotes "" to etag values.
    // https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html
    // std::string e_tag = "\"" + object_metadata->get_md5() + "\"";

    // request->set_out_header_value("Last-Modified",
    //                               object_metadata->get_last_modified_gmt());
    // request->set_out_header_value("ETag", e_tag);
    // s3_log(S3_LOG_INFO, request_id, "e_tag= %s", e_tag.c_str());
    // request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value(
        "Content-Length", std::to_string(get_requested_content_length()));
    // for (auto it : object_metadata->get_user_attributes()) {
    //   request->set_out_header_value(it.first, it.second);
    // }
    if (!request->get_header_value("Range").empty()) {
      std::ostringstream content_range_stream;
      content_range_stream << "bytes " << first_byte_offset_to_read << "-"
                           << last_byte_offset_to_read << "/" << content_length;
      request->set_out_header_value("Content-Range",
                                    content_range_stream.str());
      // Partial Content
      request->send_reply_start(S3HttpSuccess206);
    } else {
      request->send_reply_start(S3HttpSuccess200);
    }
    read_object_reply_started = true;
  } else {
    // s3_timer.resume();
  }
  s3_log(S3_LOG_DEBUG, request_id, "Earlier data_sent_to_client = %zu bytes.\n",
         data_sent_to_client);

  char* data = NULL;
  size_t length = 0;
  size_t requested_content_length = get_requested_content_length();
  s3_log(S3_LOG_DEBUG, request_id,
         "object requested content length size(%zu).\n",
         requested_content_length);
  length = clovis_reader->get_first_block(&data);

  while (length > 0) {
    size_t read_data_start_offset = 0;
    blocks_already_read++;
    if (data_sent_to_client == 0) {
      // get starting offset from the block,
      // condition true for only statring block read object.
      // this is to set get first offset byte from initial read block
      // eg: read_data_start_offset will be set to 1000 on initial read block
      // for a given range 1000-1500 to read from 2mb object
      read_data_start_offset =
          first_byte_offset_to_read %
          S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(
              layout_id);
      length -= read_data_start_offset;
    }
    // to read number of bytes from final read block of read object
    // that is requested content length is lesser than the sum of data has been
    // sent to client and current read block size
    if ((data_sent_to_client + length) >= requested_content_length) {
      // length will have the size of remaining byte to sent
      length = requested_content_length - data_sent_to_client;
    }
    data_sent_to_client += length;
    s3_log(S3_LOG_DEBUG, request_id, "Sending %zu bytes to client.\n", length);
    request->send_reply_body(data + read_data_start_offset, length);
    // s3_perf_count_outcoming_bytes(length);
    length = clovis_reader->get_next_block(&data);
  }
  //   s3_timer.stop();

  if (data_sent_to_client != requested_content_length) {
    read_object_data();
  } else {
    // const auto mss = s3_timer.elapsed_time_in_millisec();
    // LOG_PERF("get_object_send_data_ms", request_id.c_str(), mss);
    // s3_stats_timing("get_object_send_data", mss);

    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroGetObjectAction::read_object_data_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Failed to read object data from clovis\n");
  // set error only when reply is not started
  if (!read_object_reply_started) {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void MeroGetObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (reject_if_shutting_down()) {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      // Send response with 'Service Unavailable' code.
      s3_log(S3_LOG_DEBUG, request_id,
             "sending 'Service Unavailable' response...\n");
      S3Error error("ServiceUnavailable", request->get_request_id(),
                    request->get_key_name());
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
                  request->get_key_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (clovis_reader &&
             clovis_reader->get_state() == S3ClovisReaderOpState::success) {
    request->send_reply_end();
  } else {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      S3Error error("InternalError", request->get_request_id(),
                    request->get_key_name());
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
}
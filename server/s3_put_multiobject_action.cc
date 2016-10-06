/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar        <rajesh.nambiar@seagate.com>
 * Original creation date: 22-Jan-2016
 */

#include "s3_put_multiobject_action.h"
#include "s3_option.h"
#include "s3_error_codes.h"
#include "s3_perf_logger.h"
#include "s3_log.h"

S3PutMultiObjectAction::S3PutMultiObjectAction(std::shared_ptr<S3RequestObject> req) :
    S3Action(req), total_data_to_stream(0),
    auth_failed(false), write_failed(false),
    clovis_write_in_progress(false), clovis_write_completed(false),
    auth_in_progress(false), auth_completed(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  part_number = get_part_number();
  upload_id = request->get_query_string_value("uploadId");
  if (request->is_chunked() && !S3Option::get_instance()->is_auth_disabled()) {
    clear_tasks(); // remove default auth
    // Add chunk style auth
    add_task(std::bind( &S3Action::start_chunk_authentication, this ));
  } else {
    // auth is disabled, so assume its done.
    auth_completed = true;
  }
  setup_steps();
}

void S3PutMultiObjectAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");

  add_task(std::bind( &S3PutMultiObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PutMultiObjectAction::fetch_multipart_metadata, this ));
  if (part_number != 1) {
    add_task(std::bind( &S3PutMultiObjectAction::fetch_firstpart_info, this ));
  }
  add_task(std::bind( &S3PutMultiObjectAction::compute_part_offset, this ));
  add_task(std::bind( &S3PutMultiObjectAction::initiate_data_streaming, this ));
  add_task(std::bind( &S3PutMultiObjectAction::save_metadata, this ));
  add_task(std::bind( &S3PutMultiObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutMultiObjectAction::chunk_auth_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    auth_completed = true;
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (clovis_write_completed) {
    next();
  } else {
    // wait for write to complete. do nothing here.
    auth_completed = true;
  }
}

void S3PutMultiObjectAction::chunk_auth_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    auth_failed = true;
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  auth_failed = true;
  if (clovis_write_in_progress) {
    // Do nothing, handle after write returns
  } else {
    send_response_to_s3_client();
  }
}

void S3PutMultiObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PutMultiObjectAction::next, this), std::bind( &S3PutMultiObjectAction::fetch_bucket_info_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_ERROR, "Bucket does not exists\n");
  send_response_to_s3_client();
}

void S3PutMultiObjectAction::fetch_multipart_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  object_multipart_metadata = std::make_shared<S3ObjectMetadata>(
      request, bucket_metadata->get_multipart_index_oid(), true, upload_id);
  object_multipart_metadata->load(std::bind( &S3PutMultiObjectAction::next, this), std::bind( &S3PutMultiObjectAction::fetch_multipart_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::fetch_multipart_failed() {
  //Log error
  s3_log(S3_LOG_ERROR, "Failed to retrieve multipart upload metadata\n");
  send_response_to_s3_client();
}

void S3PutMultiObjectAction::fetch_firstpart_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata = std::make_shared<S3PartMetadata>(
      request, object_multipart_metadata->get_part_index_oid(), upload_id, 1);
  part_metadata->load(std::bind( &S3PutMultiObjectAction::next, this), std::bind( &S3PutMultiObjectAction::fetch_firstpart_info_failed, this), 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::fetch_firstpart_info_failed() {
  s3_log(S3_LOG_WARN, "Part 1 metadata doesn't exist, cannot determine \"consistent\" part size\n");
  send_response_to_s3_client();
}

void S3PutMultiObjectAction::compute_part_offset() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  size_t offset = 0;
  if (part_number != 1) {
    size_t part_one_size = part_metadata->get_content_length();
    s3_log(S3_LOG_DEBUG, "Part size = %zu for part_number = %d\n", request->get_content_length(), part_number);
    // Calculate offset
    offset = (part_number - 1) * part_one_size;
    s3_log(S3_LOG_DEBUG, "Offset for clovis write = %zu\n", offset);
  }
  // Create writer to write from given offset as per the partnumber
  clovis_writer = std::make_shared<S3ClovisWriter>(request, object_multipart_metadata->get_oid(), offset);
  next();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  total_data_to_stream = request->get_data_length();
  // request->resume();

  if (request->is_chunked() && !S3Option::get_instance()->is_auth_disabled()) {
    get_auth_client()->init_chunk_auth_cycle(std::bind( &S3PutMultiObjectAction::chunk_auth_successful, this), std::bind( &S3PutMultiObjectAction::chunk_auth_failed, this));
  }

  if (total_data_to_stream == 0) {
    next();  // Zero size object.
  } else {
    if (request->has_all_body_content()) {
      write_object(request->get_buffered_input());
    } else {
      s3_log(S3_LOG_DEBUG, "We do not have all the data, so start listening....\n");
      // Start streaming, logically pausing action till we get data.
      request->listen_for_incoming_data(
          std::bind(&S3PutMultiObjectAction::consume_incoming_content, this),
          S3Option::get_instance()->get_clovis_write_payload_size()
        );
    }
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (!clovis_write_in_progress) {
    if (request->get_buffered_input().is_freezed() ||
        request->get_buffered_input().length() >=
            S3Option::get_instance()->get_clovis_write_payload_size()) {
      write_object(request->get_buffered_input());
    }
  }
  if (!request->get_buffered_input().is_freezed() &&
      request->get_buffered_input().length() >=
      (S3Option::get_instance()->get_clovis_write_payload_size() * S3Option::get_instance()->get_read_ahead_multiple())) {
    s3_log(S3_LOG_DEBUG, "Pausing with Buffered length = %zu\n", request->get_buffered_input().length());
    request->pause();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::write_object(S3AsyncBufferContainer& buffer) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (request->is_chunked()) {
    // Also send any ready chunk data for auth
    while (request->is_chunk_detail_ready()) {
      S3ChunkDetail detail = request->pop_chunk_detail();
      s3_log(S3_LOG_DEBUG, "Using chunk details for auth:\n");
      detail.debug_dump();
      if (!S3Option::get_instance()->is_auth_disabled()) {
        if (detail.get_size() == 0) {
          // Last chunk is size 0
          get_auth_client()->add_last_checksum_for_chunk(detail.get_signature(), detail.get_payload_hash());
        } else {
          get_auth_client()->add_checksum_for_chunk(detail.get_signature(), detail.get_payload_hash());
        }
        auth_in_progress = true;  // this triggers auth
      }
    }
  }

  clovis_writer->write_content(std::bind( &S3PutMultiObjectAction::write_object_successful, this), std::bind( &S3PutMultiObjectAction::write_object_failed, this), buffer);

  clovis_write_in_progress = true;

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::write_object_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    clovis_write_in_progress = false;
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  s3_log(S3_LOG_DEBUG, "Write successful\n");
  clovis_write_in_progress = false;
  if (request->is_chunked()) {
    if (auth_failed) {
      send_response_to_s3_client();
      return;
    }
  }
  if (/* buffered data len is at least equal max we can write to clovis in one write */
      request->get_buffered_input().length() > S3Option::get_instance()->get_clovis_write_payload_size()
      || /* we have all the data buffered and ready to write */
      (request->get_buffered_input().is_freezed() && request->get_buffered_input().length() > 0)) {
    write_object(request->get_buffered_input());
  } else if (request->get_buffered_input().is_freezed() && request->get_buffered_input().length() == 0) {
    clovis_write_completed = true;
    if (request->is_chunked()) {
      if (auth_completed) {
        next();
      }  // else wait for auth to complete
    } else {
      next();
    }
  } else if (!request->get_buffered_input().is_freezed()) {
    // else we wait for more incoming data
    request->resume();
  }
}

void S3PutMultiObjectAction::write_object_failed() {
  s3_log(S3_LOG_ERROR, "Write to clovis failed\n");
  clovis_write_in_progress = false;
  clovis_write_completed = true;
  if (request->is_chunked()) {
    write_failed = true;
    if (!auth_in_progress) {
      send_response_to_s3_client();
    }
  } else {
    send_response_to_s3_client();
  }
}

void S3PutMultiObjectAction::save_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata = std::make_shared<S3PartMetadata>(request, upload_id, part_number);
  part_metadata->set_content_length(request->get_data_length_str());
  part_metadata->set_md5(clovis_writer->get_content_md5());
  for (auto it: request->get_in_headers_copy()) {
    if(it.first.find("x-amz-meta-") != std::string::npos) {
      part_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  part_metadata->save(std::bind( &S3PutMultiObjectAction::next, this), std::bind( &S3PutMultiObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutMultiObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (reject_if_shutting_down()) {
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
    request->set_out_header_value("Retry-After", "1");
    request->send_response(S3HttpFailed503);
  } else if (request->is_chunked() && auth_failed) {
    // Invalid Bucket Name
    S3Error error("SignatureDoesNotMatch", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata && (bucket_metadata->get_state() == S3BucketMetadataState::missing)) {
    s3_log(S3_LOG_ERROR, "Missing bucket for multipart upload, upload id = %s, request id = %s object uri = %s\n",upload_id.c_str(), request->get_request_id().c_str(), request->get_object_uri().c_str());
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_multipart_metadata && (object_multipart_metadata->get_state() == S3ObjectMetadataState::missing)) {
    // The multipart upload may have been aborted
    s3_log(S3_LOG_WARN, "The metadata of multipart upload doesn't exist, upload id = %s request id = %s object uri = %s\n",
           upload_id.c_str(), request->get_request_id().c_str(), request->get_object_uri().c_str());
    S3Error error("NoSuchUpload", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (part_metadata && (part_metadata->get_state() == S3PartMetadataState::missing)) {
    // May happen if part 2/3... comes before part 1, in that case those part
    // upload need to be retried(by that time part 1 meta data will get in)
    s3_log(S3_LOG_WARN, "Part one metadata is not available, asking client to retry, upload id = %s request id = %s object uri = %s\n",
         upload_id.c_str(),  request->get_request_id().c_str(), request->get_object_uri().c_str());
    S3Error error("ServiceUnavailable", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    // Let the client retry after 1 second delay
    request->set_out_header_value("Retry-After", "1");
    request->send_response(error.get_http_status_code(), response_xml);

  } else if (clovis_writer && (clovis_writer->get_state() == S3ClovisWriterOpState::failed)) {
    s3_log(S3_LOG_ERROR, "Clovis failed to write for multipart upload, upload id = %s request id = %s object uri = %s",
           upload_id.c_str(), request->get_request_id().c_str(), request->get_object_uri().c_str());
    S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (part_metadata && (part_metadata->get_state() == S3PartMetadataState::saved)) {
    request->set_out_header_value("ETag", clovis_writer->get_content_md5());

    request->send_response(S3HttpSuccess200);
  } else {
    s3_log(S3_LOG_ERROR, "Internal error upload id = %s request id = %s object uri = %s\n",
           upload_id.c_str(), request->get_request_id().c_str(), request->get_object_uri().c_str());
    S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  request->resume();

  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

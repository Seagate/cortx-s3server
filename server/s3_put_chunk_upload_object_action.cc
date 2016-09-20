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
 * Original creation date: 17-Mar-2016
 */

#include "s3_put_chunk_upload_object_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_uri_to_mero_oid.h"

S3PutChunkUploadObjectAction::S3PutChunkUploadObjectAction(std::shared_ptr<S3RequestObject> req) :
    S3Action(req), total_data_to_stream(0),
    auth_failed(false), write_failed(false),
    clovis_write_in_progress(false), clovis_write_completed(false),
    auth_in_progress(false), auth_completed(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  clear_tasks(); // remove default auth
  if (!S3Option::get_instance()->is_auth_disabled()) {
    // Add chunk style auth
    add_task(std::bind( &S3Action::start_chunk_authentication, this ));
  } else {
    // auth is disabled, so assume its done.
    auth_completed = true;
  }
  S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
  tried_count = 0;
  salt = "uri_salt_";
  setup_steps();
}

void S3PutChunkUploadObjectAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");

  add_task(std::bind( &S3PutChunkUploadObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PutChunkUploadObjectAction::create_object, this ));
  add_task(std::bind( &S3PutChunkUploadObjectAction::initiate_data_streaming, this ));
  add_task(std::bind( &S3PutChunkUploadObjectAction::save_metadata, this ));
  add_task(std::bind( &S3PutChunkUploadObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutChunkUploadObjectAction::chunk_auth_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    auth_completed = true;
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (clovis_write_completed) {
    if (write_failed)
      rollback_start();
    else
      next();
  } else {
    // wait for write to complete. do nothing here.
    auth_completed = true;
  }
}

void S3PutChunkUploadObjectAction::chunk_auth_failed() {
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
    if (write_failed)
      rollback_start();
    else
      send_response_to_s3_client();
  }
}

void S3PutChunkUploadObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PutChunkUploadObjectAction::next, this), std::bind( &S3PutChunkUploadObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::create_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    create_object_timer.start();
    clovis_writer = std::make_shared<S3ClovisWriter>(request, oid);
    clovis_writer->create_object(std::bind( &S3PutChunkUploadObjectAction::next, this), std::bind( &S3PutChunkUploadObjectAction::create_object_failed, this));
  } else {
    s3_log(S3_LOG_WARN, "Bucket [%s] not found\n", request->get_bucket_name().c_str());
    request->resume();
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::create_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    // If object exists, it may be due to the actual existance of object or due
    // to oid collision
    if (tried_count) {  // No need of lookup of metadata in case if it was oid
                        // collision before
      collision_detected();
    } else {
      object_metadata = std::make_shared<S3ObjectMetadata>(
          request, bucket_metadata->get_object_list_index_oid());
      // Lookup metadata, if the object doesn't exist then its collision, do
      // collision resolution
      // If object exist in metadata then we overwrite it
      object_metadata->load(
          std::bind(&S3PutChunkUploadObjectAction::next, this),
          std::bind(&S3PutChunkUploadObjectAction::collision_detected, this));
    }
  } else {
    create_object_timer.stop();
    LOG_PERF("create_object_failed_ms", create_object_timer.elapsed_time_in_millisec());
    s3_log(S3_LOG_WARN, "Create object failed.\n");

    request->resume();
    // Any other error report failure.
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::collision_detected() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (object_metadata->get_state() == S3ObjectMetadataState::missing &&
      tried_count < MAX_COLLISION_TRY) {
    s3_log(S3_LOG_INFO, "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid();
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, "Object ID collision happened %d times for uri %s\n",
             tried_count, request->get_object_uri().c_str());
    }
    create_object();
  } else {
    if (tried_count > MAX_COLLISION_TRY) {
      s3_log(S3_LOG_ERROR,
             "Failed to resolve object id collision %d times for uri %s\n",
             tried_count, request->get_object_uri().c_str());
    }
    send_response_to_s3_client();
  }
}

void S3PutChunkUploadObjectAction::create_new_oid() {
  std::string salted_uri =
      request->get_object_uri() + salt + std::to_string(tried_count);
  S3UriToMeroOID(salted_uri.c_str(), &oid);
  return;
}

void S3PutChunkUploadObjectAction::rollback_create() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  clovis_writer->delete_object(
      std::bind(&S3PutChunkUploadObjectAction::rollback_next, this),
      std::bind(&S3PutChunkUploadObjectAction::rollback_create_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::rollback_create_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    rollback_next();
  } else {
    // Log rollback failure.
    s3_log(S3_LOG_WARN, "Deletion of object failed\n");
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  // mark rollback point
  add_task_rollback(
      std::bind(&S3PutChunkUploadObjectAction::rollback_create, this));

  create_object_timer.stop();
  LOG_PERF("create_object_successful_ms", create_object_timer.elapsed_time_in_millisec());

  total_data_to_stream = request->get_data_length();
  if (!S3Option::get_instance()->is_auth_disabled()) {
    get_auth_client()->init_chunk_auth_cycle(std::bind( &S3PutChunkUploadObjectAction::chunk_auth_successful, this), std::bind( &S3PutChunkUploadObjectAction::chunk_auth_failed, this));
  }

  if (total_data_to_stream == 0) {
    next();  // Zero size object.
  } else {
    if (request->has_all_body_content()) {
      s3_log(S3_LOG_DEBUG, "We have all the data, so just write it.\n");
      write_object(request->get_buffered_input());
    } else {
      s3_log(S3_LOG_DEBUG, "We do not have all the data, start listening...\n");
      // Start streaming, logically pausing action till we get data.
      request->listen_for_incoming_data(
          std::bind(&S3PutChunkUploadObjectAction::consume_incoming_content, this),
          S3Option::get_instance()->get_clovis_write_payload_size()
        );
    }
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (!clovis_write_in_progress) {
    write_object(request->get_buffered_input());
  }
  if (!request->get_buffered_input().is_freezed() &&
      request->get_buffered_input().length() >=
      (S3Option::get_instance()->get_clovis_write_payload_size() * S3Option::get_instance()->get_read_ahead_multiple())) {
    s3_log(S3_LOG_DEBUG, "Pausing with Buffered length = %zu\n", request->get_buffered_input().length());
    request->pause();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::write_object(S3AsyncBufferContainer& buffer) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
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
  clovis_write_in_progress = true;

  clovis_writer->write_content(std::bind( &S3PutChunkUploadObjectAction::write_object_successful, this), std::bind( &S3PutChunkUploadObjectAction::write_object_failed, this), buffer);

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::write_object_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    clovis_write_in_progress = false;
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  s3_log(S3_LOG_DEBUG, "Write to clovis successful\n");
  clovis_write_in_progress = false;
  if (auth_failed) {
    // Trigger rollback to undo changes done and report error
    rollback_start();
    return;
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
  } if (!request->get_buffered_input().is_freezed()) {
    // else we wait for more incoming data
    request->resume();
  }
}

void S3PutChunkUploadObjectAction::write_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    clovis_write_in_progress = false;
    write_failed = true;
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  s3_log(S3_LOG_WARN, "Failed writing to clovis.\n");
  clovis_write_in_progress = false;
  write_failed = true;
  clovis_write_completed = true;
  if (!auth_in_progress) {
    // Trigger rollback to undo changes done and report error
    rollback_start();
  }
}

void S3PutChunkUploadObjectAction::save_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // xxx set attributes & save
  object_metadata = std::make_shared<S3ObjectMetadata>(
      request, bucket_metadata->get_object_list_index_oid());
  object_metadata->set_content_length(request->get_data_length_str());
  object_metadata->set_md5(clovis_writer->get_content_md5());
  object_metadata->set_oid(clovis_writer->get_oid());
  for (auto it: request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      s3_log(S3_LOG_DEBUG, "Writing user metadata on object: [%s] -> [%s]\n",
          it.first.c_str(), it.second.c_str());
      object_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  object_metadata->save(
      std::bind(&S3PutChunkUploadObjectAction::next, this),
      std::bind(&S3PutChunkUploadObjectAction::rollback_start, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutChunkUploadObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (reject_if_shutting_down()) {
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
    request->set_out_header_value("Retry-After", "1");
    request->send_response(S3HttpFailed503);
  } else if (auth_failed) {
    // Invalid Bucket Name
    S3Error error("SignatureDoesNotMatch", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (clovis_writer && clovis_writer->get_state() == S3ClovisWriterOpState::failed) {
    S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata && object_metadata->get_state() == S3ObjectMetadataState::saved) {
    request->set_out_header_value("ETag", clovis_writer->get_content_md5());

    request->send_response(S3HttpSuccess200);
  } else {
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

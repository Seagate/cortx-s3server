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

#include "s3_put_object_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_uri_to_mero_oid.h"

#define MAX_COLLISION_TRY 20

S3PutObjectAction::S3PutObjectAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req), total_data_to_stream(0), write_in_progress(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
  tried_count = 0;
  salt = "uri_salt_";
  setup_steps();
}

void S3PutObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3PutObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3PutObjectAction::create_object, this));
  add_task(std::bind(&S3PutObjectAction::initiate_data_streaming, this));
  add_task(std::bind(&S3PutObjectAction::save_metadata, this));
  add_task(std::bind(&S3PutObjectAction::send_response_to_s3_client, this));
  // ...
}

void S3PutObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind(&S3PutObjectAction::next, this),
                        std::bind(&S3PutObjectAction::next, this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_fetch_bucket_info_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::create_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    create_object_timer.start();
    if (tried_count == 0) {
      clovis_writer = std::make_shared<S3ClovisWriter>(request, oid);
    } else {
      clovis_writer->set_oid(oid);
    }
    clovis_writer->create_object(
        std::bind(&S3PutObjectAction::next, this),
        std::bind(&S3PutObjectAction::create_object_failed, this));
  } else {
    s3_log(S3_LOG_WARN, "Bucket [%s] not found\n",
           request->get_bucket_name().c_str());
    send_response_to_s3_client();
  }

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::create_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    m0_uint128 object_list_oid = bucket_metadata->get_object_list_index_oid();

    if (tried_count == 0) {
      object_metadata = std::make_shared<S3ObjectMetadata>(
          request, bucket_metadata->get_object_list_index_oid());
    }

    // If object exists, it may be due to the actual existance of object or due
    // to oid collision
    if (tried_count ||
        (object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL)) {
      // No need of lookup of metadata in case if it was oid collision before or
      // in case
      // if object list index doesn't exist
      collision_detected();
    } else {
      // Lookup metadata, if the object doesn't exist then its collision, do
      // collision resolution
      // If object exist in metadata then we overwrite it
      object_metadata->load(
          std::bind(&S3PutObjectAction::next, this),
          std::bind(&S3PutObjectAction::collision_detected, this));
    }
  } else {
    create_object_timer.stop();
    LOG_PERF("create_object_failed_ms",
             create_object_timer.elapsed_time_in_millisec());
    s3_stats_timing("create_object_failed",
                    create_object_timer.elapsed_time_in_millisec());
    s3_log(S3_LOG_WARN, "Create object failed.\n");

    // Any other error report failure.
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047004001</event_code>
 *    <application>S3 Server</application>
 *    <submodule>Collision handling</submodule>
 *    <description>Collision resolution failed</description>
 *    <audience>Service</audience>
 *    <details>
 *      Failed to resolve object/index id collision.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files.
 *      Contact development team for further investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

void S3PutObjectAction::collision_detected() {
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (tried_count < MAX_COLLISION_TRY) {
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
    s3_log(S3_LOG_ERROR,
           "Failed to resolve object id collision %d times for uri %s\n",
           tried_count, request->get_object_uri().c_str());
    s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
           S3_IEM_COLLISION_RES_FAIL_JSON);
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::create_new_oid() {
  std::string salted_uri =
      request->get_object_uri() + salt + std::to_string(tried_count);
  S3UriToMeroOID(salted_uri.c_str(), &oid);
  return;
}

void S3PutObjectAction::rollback_create() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  clovis_writer->delete_object(
      std::bind(&S3PutObjectAction::rollback_next, this),
      std::bind(&S3PutObjectAction::rollback_create_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::rollback_create_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    rollback_next();
  } else {
    // Log rollback failure.
    s3_log(S3_LOG_WARN, "Deletion of object failed\n");
    s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
           S3_IEM_DELETE_OBJ_FAIL_JSON);
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  create_object_timer.stop();
  LOG_PERF("create_object_successful_ms",
           create_object_timer.elapsed_time_in_millisec());
  s3_stats_timing("create_object_success",
                  create_object_timer.elapsed_time_in_millisec());

  // mark rollback point
  add_task_rollback(std::bind(&S3PutObjectAction::rollback_create, this));

  total_data_to_stream = request->get_content_length();

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
          std::bind(&S3PutObjectAction::consume_incoming_content, this),
          S3Option::get_instance()->get_clovis_write_payload_size());
    }
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_consume_incoming_content_shutdown_fail");
  // Resuming the action since we have data.
  if (!write_in_progress) {
    if (request->get_buffered_input()->is_freezed() ||
        request->get_buffered_input()->get_content_length() >=
            S3Option::get_instance()->get_clovis_write_payload_size()) {
      write_object(request->get_buffered_input());
    }
  }
  if (!request->get_buffered_input()->is_freezed() &&
      request->get_buffered_input()->get_content_length() >=
          (S3Option::get_instance()->get_clovis_write_payload_size() *
           S3Option::get_instance()->get_read_ahead_multiple())) {
    s3_log(S3_LOG_DEBUG, "Pausing with Buffered length = %zu\n",
           request->get_buffered_input()->get_content_length());
    request->pause();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::write_object(
    std::shared_ptr<S3AsyncBufferOptContainer> buffer) {
  s3_log(S3_LOG_DEBUG, "Entering with buffer length = %zu\n",
         buffer->get_content_length());

  clovis_writer->write_content(
      std::bind(&S3PutObjectAction::write_object_successful, this),
      std::bind(&S3PutObjectAction::write_object_failed, this), buffer);

  write_in_progress = true;
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::write_object_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  s3_log(S3_LOG_DEBUG, "Write to clovis successful\n");
  write_in_progress = false;
  if (/* buffered data len is at least equal to max we can write to clovis in
         one write */
      request->get_buffered_input()->get_content_length() >
          S3Option::get_instance()
              ->get_clovis_write_payload_size() || /* we have all the data
                                                      buffered and ready to
                                                      write */
      (request->get_buffered_input()->is_freezed() &&
       request->get_buffered_input()->get_content_length() > 0)) {
    write_object(request->get_buffered_input());
  } else if (request->get_buffered_input()->is_freezed() &&
             request->get_buffered_input()->get_content_length() == 0) {
    next();
  } else if (!request->get_buffered_input()->is_freezed()) {
    // else we wait for more incoming data
    request->resume();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::write_object_failed() {
  s3_log(S3_LOG_WARN, "Failed writing to clovis.\n");
  write_in_progress = false;

  // Trigger rollback to undo changes done and report error
  rollback_start();
}

void S3PutObjectAction::save_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");
  // xxx set attributes & save
  object_metadata = std::make_shared<S3ObjectMetadata>(
      request, bucket_metadata->get_object_list_index_oid());
  object_metadata->set_content_length(request->get_data_length_str());
  object_metadata->set_md5(clovis_writer->get_content_md5());
  object_metadata->set_oid(clovis_writer->get_oid());
  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      s3_log(S3_LOG_DEBUG, "Writing user metadata on object: [%s] -> [%s]\n",
             it.first.c_str(), it.second.c_str());
      object_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  object_metadata->save(std::bind(&S3PutObjectAction::next, this),
                        std::bind(&S3PutObjectAction::rollback_start, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (reject_if_shutting_down()) {
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
    S3Error error("ServiceUnavailable", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Retry-After", "1");

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::failed) {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (clovis_writer &&
             clovis_writer->get_state() == S3ClovisWriterOpState::failed) {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata &&
             object_metadata->get_state() == S3ObjectMetadataState::saved) {
    request->set_out_header_value("ETag", clovis_writer->get_content_md5());

    request->send_response(S3HttpSuccess200);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  request->resume();

  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

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
#include "s3_clovis_layout.h"
#include "s3_common.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_uri_to_mero_oid.h"

S3PutObjectAction::S3PutObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3ClovisWriterFactory> clovis_s3_factory)
    : S3Action(req), total_data_to_stream(0), write_in_progress(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id, "S3 API: Put Object. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  if (clovis_api) {
    s3_clovis_api = clovis_api;
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }

  S3UriToMeroOID(s3_clovis_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);
  // Note valid value is set during create object
  layout_id = -1;

  tried_count = 0;
  salt = "uri_salt_";

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
    clovis_writer_factory = clovis_s3_factory;
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }

  setup_steps();
}

void S3PutObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3PutObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3PutObjectAction::fetch_object_info, this));
  add_task(std::bind(&S3PutObjectAction::create_object, this));
  add_task(std::bind(&S3PutObjectAction::initiate_data_streaming, this));
  add_task(std::bind(&S3PutObjectAction::save_metadata, this));
  add_task(std::bind(&S3PutObjectAction::delete_old_object_if_present, this));
  add_task(std::bind(&S3PutObjectAction::send_response_to_s3_client, this));
  // ...
}

void S3PutObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);

  bucket_metadata->load(std::bind(&S3PutObjectAction::next, this),
                        std::bind(&S3PutObjectAction::next, this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_fetch_bucket_info_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::fetch_object_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Found bucket metadata\n");
    struct m0_uint128 object_list_oid =
        bucket_metadata->get_object_list_index_oid();
    if (object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL) {
      // There is no object list index, hence object doesn't exist
      s3_log(S3_LOG_DEBUG, request_id, "No existing object, Create it.\n");
      next();
    } else {
      object_metadata = object_metadata_factory->create_object_metadata_obj(
          request, object_list_oid);

      object_metadata->load(
          std::bind(&S3PutObjectAction::fetch_object_info_status, this),
          std::bind(&S3PutObjectAction::next, this));
    }
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
    send_response_to_s3_client();
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::fetch_object_info_status() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "S3ObjectMetadataState::present\n");
    old_object_oid = object_metadata->get_oid();
    old_layout_id = object_metadata->get_layout_id();
    create_new_oid(old_object_oid);
    next();
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "S3ObjectMetadataState::missing\n");
    next();
  } else if (object_metadata->get_state() ==
             S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Object metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to look up metadata.\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  create_object_timer.start();
  if (tried_count == 0) {
    clovis_writer =
        clovis_writer_factory->create_clovis_writer(request, new_object_oid);
  } else {
    clovis_writer->set_oid(new_object_oid);
  }

  layout_id = S3ClovisLayoutMap::get_instance()->get_layout_for_object_size(
      request->get_content_length());

  clovis_writer->create_object(
      std::bind(&S3PutObjectAction::next, this),
      std::bind(&S3PutObjectAction::create_object_failed, this), layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    collision_detected();
  } else if (clovis_writer->get_state() ==
             S3ClovisWriterOpState::failed_to_launch) {
    create_object_timer.stop();
    LOG_PERF("create_object_failed_ms",
             create_object_timer.elapsed_time_in_millisec());
    s3_stats_timing("create_object_failed",
                    create_object_timer.elapsed_time_in_millisec());
    s3_log(S3_LOG_ERROR, request_id, "Create object failed.\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    create_object_timer.stop();
    LOG_PERF("create_object_failed_ms",
             create_object_timer.elapsed_time_in_millisec());
    s3_stats_timing("create_object_failed",
                    create_object_timer.elapsed_time_in_millisec());
    s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");

    // Any other error report failure.
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
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
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (tried_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id, "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid(new_object_oid);
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, request_id,
             "Object ID collision happened %d times for uri %s\n", tried_count,
             request->get_object_uri().c_str());
    }
    create_object();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Exceeded maximum collision retry attempts."
           "Collision occurred %d times for uri %s\n",
           tried_count, request->get_object_uri().c_str());
    s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
           S3_IEM_COLLISION_RES_FAIL_JSON);
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::create_new_oid(struct m0_uint128 current_oid) {
  int salt_counter = 0;
  std::string salted_uri;
  do {
    salted_uri = request->get_object_uri() + salt +
                 std::to_string(salt_counter) + std::to_string(tried_count);

    S3UriToMeroOID(s3_clovis_api, salted_uri.c_str(), request_id,
                   &new_object_oid);

    ++salt_counter;
  } while ((new_object_oid.u_hi == current_oid.u_hi) &&
           (new_object_oid.u_lo == current_oid.u_lo));

  return;
}

void S3PutObjectAction::rollback_create() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  clovis_writer->set_oid(new_object_oid);
  if (object_metadata->get_state() == S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Save object metadata failed due to prelaunch failure\n");
    set_s3_error("ServiceUnavailable");
  }
  clovis_writer->delete_object(
      std::bind(&S3PutObjectAction::rollback_next, this),
      std::bind(&S3PutObjectAction::rollback_create_failed, this), layout_id);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::rollback_create_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    rollback_next();
  } else if (clovis_writer->get_state() ==
             S3ClovisWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Rollback: Deletion of object with oid "
           "%" SCNx64 " : %" SCNx64 " failed, it may remain stale in Mero\n",
           new_object_oid.u_hi, new_object_oid.u_lo);
    set_s3_error("ServiceUnavailable");
    s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
           S3_IEM_DELETE_OBJ_FAIL_JSON);
    send_response_to_s3_client();
  } else {
    // Log rollback failure.
    s3_log(S3_LOG_ERROR, request_id,
           "Rollback: Deletion of object with oid "
           "%" SCNx64 " : %" SCNx64 " failed, it may remain stale in Mero\n",
           new_object_oid.u_hi, new_object_oid.u_lo);
    set_s3_error("InternalError");
    s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
           S3_IEM_DELETE_OBJ_FAIL_JSON);
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
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
      s3_log(S3_LOG_DEBUG, request_id,
             "We have all the data, so just write it.\n");
      write_object(request->get_buffered_input());
    } else {
      s3_log(S3_LOG_DEBUG, request_id,
             "We do not have all the data, start listening...\n");
      // Start streaming, logically pausing action till we get data.
      request->listen_for_incoming_data(
          std::bind(&S3PutObjectAction::consume_incoming_content, this),
          S3Option::get_instance()->get_clovis_write_payload_size(layout_id));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_consume_incoming_content_shutdown_fail");
  // Resuming the action since we have data.
  if (!write_in_progress) {
    if (request->get_buffered_input()->is_freezed() ||
        request->get_buffered_input()->get_content_length() >=
            S3Option::get_instance()->get_clovis_write_payload_size(
                layout_id)) {
      write_object(request->get_buffered_input());
    }
  }
  if (!request->get_buffered_input()->is_freezed() &&
      request->get_buffered_input()->get_content_length() >=
          (S3Option::get_instance()->get_clovis_write_payload_size(layout_id) *
           S3Option::get_instance()->get_read_ahead_multiple())) {
    s3_log(S3_LOG_DEBUG, request_id, "Pausing with Buffered length = %zu\n",
           request->get_buffered_input()->get_content_length());
    request->pause();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::write_object(
    std::shared_ptr<S3AsyncBufferOptContainer> buffer) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with buffer length = %zu\n",
         buffer->get_content_length());

  clovis_writer->write_content(
      std::bind(&S3PutObjectAction::write_object_successful, this),
      std::bind(&S3PutObjectAction::write_object_failed, this), buffer);

  write_in_progress = true;
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::write_object_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Write to clovis successful\n");
  write_in_progress = false;

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (/* buffered data len is at least equal to max we can write to clovis in
         one write */
      request->get_buffered_input()->get_content_length() >=
          S3Option::get_instance()->get_clovis_write_payload_size(
              layout_id) || /* we have all the data buffered and ready to
                               write */
      (request->get_buffered_input()->is_freezed() &&
       request->get_buffered_input()->get_content_length() > 0)) {
    write_object(request->get_buffered_input());
  } else if (request->get_buffered_input()->is_freezed() &&
             request->get_buffered_input()->get_content_length() == 0) {
    next();
  }
  if (!request->get_buffered_input()->is_freezed()) {
    // else we wait for more incoming data
    request->resume();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::write_object_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed writing to clovis.\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
    s3_log(S3_LOG_ERROR, request_id, "write_object_failed failure\n");
  } else {
    set_s3_error("InternalError");
  }
  write_in_progress = false;

  // Trigger rollback to undo changes done and report error
  rollback_start();
}

void S3PutObjectAction::save_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");
  // xxx set attributes & save
  if (!object_metadata) {
    object_metadata = object_metadata_factory->create_object_metadata_obj(
        request, bucket_metadata->get_object_list_index_oid());
  }

  object_metadata->set_content_length(request->get_data_length_str());
  object_metadata->set_md5(clovis_writer->get_content_md5());
  object_metadata->set_oid(clovis_writer->get_oid());
  object_metadata->set_layout_id(layout_id);

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Writing user metadata on object: [%s] -> [%s]\n",
             it.first.c_str(), it.second.c_str());
      object_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }

  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  object_metadata->save(
      std::bind(&S3PutObjectAction::next, this),
      std::bind(&S3PutObjectAction::save_object_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::save_object_metadata_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  }
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::delete_old_object_if_present() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (old_object_oid.u_hi == 0ULL && old_object_oid.u_lo == 0ULL) {
    next();
  } else {
    clovis_writer->set_oid(old_object_oid);
    clovis_writer->delete_object(
        std::bind(&S3PutObjectAction::next, this),
        std::bind(&S3PutObjectAction::delete_old_object_failed, this),
        old_layout_id);
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::delete_old_object_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Deletion of object with oid "
           "%" SCNx64 " : %" SCNx64 " failed\n",
           old_object_oid.u_hi, old_object_oid.u_lo);
    set_s3_error("ServiceUnavailable");
  } else {
  s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
         S3_IEM_DELETE_OBJ_FAIL_JSON);
  s3_log(S3_LOG_ERROR, request_id,
         "Deletion of old object with oid "
         "%" SCNx64 " : %" SCNx64 " failed\n",
         old_object_oid.u_hi, old_object_oid.u_lo);
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    if (get_s3_error_code() == "ServiceUnavailable" ||
        get_s3_error_code() == "InternalError") {
      request->set_out_header_value("Connection", "close");
    }

    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata &&
             object_metadata->get_state() == S3ObjectMetadataState::saved) {
    request->set_out_header_value("ETag", clovis_writer->get_content_md5());
    request->send_response(S3HttpSuccess200);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Connection", "close");
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  request->resume();

  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}

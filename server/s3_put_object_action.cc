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
#include <evhttp.h>
#include "s3_m0_uint128_helper.h"
#include "s3_perf_metrics.h"

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3PutObjectAction::S3PutObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3ClovisWriterFactory> clovis_s3_factory,
    std::shared_ptr<S3PutTagsBodyFactory> put_tags_body_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory)),
      total_data_to_stream(0),
      write_in_progress(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id, "S3 API: Put Object. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  old_object_oid = {0ULL, 0ULL};
  new_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  if (clovis_api) {
    s3_clovis_api = std::move(clovis_api);
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }

  S3UriToMeroOID(s3_clovis_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);
  // Note valid value is set during create object
  layout_id = -1;

  tried_count = 0;
  salt = "uri_salt_";

  if (clovis_s3_factory) {
    clovis_writer_factory = std::move(clovis_s3_factory);
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }
  if (put_tags_body_factory) {
    put_object_tag_body_factory = std::move(put_tags_body_factory);
  } else {
    put_object_tag_body_factory = std::make_shared<S3PutTagsBodyFactory>();
  }

  if (kv_writer_factory) {
    clovis_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    clovis_kv_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  setup_steps();
}

void S3PutObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  if (!request->get_header_value("x-amz-tagging").empty()) {
    ACTION_TASK_ADD(S3PutObjectAction::validate_x_amz_tagging_if_present, this);
  }
  ACTION_TASK_ADD(S3PutObjectAction::create_object, this);
  ACTION_TASK_ADD(S3PutObjectAction::initiate_data_streaming, this);
  ACTION_TASK_ADD(S3PutObjectAction::save_metadata, this);
  ACTION_TASK_ADD(S3PutObjectAction::send_response_to_s3_client, this);
  // ...
}

void S3PutObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::validate_x_amz_tagging_if_present() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::string new_object_tags = request->get_header_value("x-amz-tagging");
  s3_log(S3_LOG_DEBUG, request_id, "Received tags= %s\n",
         new_object_tags.c_str());
  if (!new_object_tags.empty()) {
    parse_x_amz_tagging_header(new_object_tags);
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::parse_x_amz_tagging_header(std::string content) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  struct evkeyvalq key_value;
  memset(&key_value, 0, sizeof(key_value));
  if (0 == evhttp_parse_query_str(content.c_str(), &key_value)) {
    char* decoded_key = NULL;
    for (struct evkeyval* header = key_value.tqh_first; header;
         header = header->next.tqe_next) {

      decoded_key = evhttp_decode_uri(header->key);
      s3_log(S3_LOG_DEBUG, request_id,
             "Successfully parsed the Key Values=%s %s", decoded_key,
             header->value);
      new_object_tags_map[decoded_key] = header->value;
    }
    validate_tags();
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::validate_tags() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  std::string xml;
  std::shared_ptr<S3PutTagBody> put_object_tag_body =
      put_object_tag_body_factory->create_put_resource_tags_body(xml,
                                                                 request_id);

  if (put_object_tag_body->validate_object_xml_tags(new_object_tags_map)) {
    next();
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::fetch_object_info_failed() {
  // Proceed to to next task, object metadata doesnt exist, will create now
  struct m0_uint128 object_list_oid =
      bucket_metadata->get_object_list_index_oid();
  if (object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL) {
    // object_list_oid is null only when bucket metadata is corrupted.
    // user has to delete and recreate the bucket again to make it work.
    s3_log(S3_LOG_ERROR, request_id, "Bucket(%s) metadata is corrupted.\n",
           request->get_bucket_name().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
    set_s3_error("MetaDataCorruption");
    send_response_to_s3_client();
  } else {
    next();
  }
}

void S3PutObjectAction::fetch_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_timer.start();
  if (tried_count == 0) {
    clovis_writer =
        clovis_writer_factory->create_clovis_writer(request, new_object_oid);
  } else {
    clovis_writer->set_oid(new_object_oid);
  }

  layout_id = S3ClovisLayoutMap::get_instance()->get_layout_for_object_size(
      request->get_content_length());

  clovis_writer->create_object(
      std::bind(&S3PutObjectAction::create_object_successful, this),
      std::bind(&S3PutObjectAction::create_object_failed, this), layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // mark rollback point
  add_task_rollback(std::bind(&S3PutObjectAction::rollback_create, this));
  if (S3Option::get_instance()->is_s3server_objectleak_tracking_enabled()) {
    add_object_oid_to_probable_dead_oid_list();
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    collision_detected();
  } else {
    s3_timer.stop();
    const auto mss = s3_timer.elapsed_time_in_millisec();
    LOG_PERF("create_object_failed_ms", request_id.c_str(), mss);
    s3_stats_timing("create_object_failed", mss);

    if (clovis_writer->get_state() == S3ClovisWriterOpState::failed_to_launch) {
      s3_log(S3_LOG_ERROR, request_id, "Create object failed.\n");
      set_s3_error("ServiceUnavailable");
    } else {
      s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
      // Any other error report failure.
      set_s3_error("InternalError");
    }
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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
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
    probable_oid_list.erase(S3M0Uint128Helper::to_string(new_object_oid));
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
    probable_oid_list.erase(S3M0Uint128Helper::to_string(new_object_oid));
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_timer.stop();
  const auto mss = s3_timer.elapsed_time_in_millisec();
  LOG_PERF("create_object_successful_ms", request_id.c_str(), mss);
  s3_stats_timing("create_object_success", mss);

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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_consume_incoming_content_shutdown_fail");
  log_timed_counter(put_timed_counter, "incoming_object_data_blocks");
  s3_perf_count_incoming_bytes(
      request->get_buffered_input()->get_content_length());
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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  std::string s_md5_got = request->get_header_value("content-md5");
  if (!s_md5_got.empty()) {
    std::string s_md5_calc = clovis_writer->get_content_md5_base64();
    s3_log(S3_LOG_DEBUG, request_id, "MD5 calculated: %s, MD5 got %s",
           s_md5_calc.c_str(), s_md5_got.c_str());

    if (s_md5_calc != s_md5_got) {
      s3_log(S3_LOG_INFO, request_id, "Content MD5 mismatch\n");
      set_s3_error("BadDigest");
      rollback_start();
      return;
    }
  }
  s3_timer.start();
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");
  // New Object or overwrite, create new metadata and release old.
  object_metadata = object_metadata_factory->create_object_metadata_obj(
      request, bucket_metadata->get_object_list_index_oid());

  object_metadata->reset_date_time_to_current();
  object_metadata->set_content_length(request->get_data_length_str());
  object_metadata->set_md5(clovis_writer->get_content_md5());
  object_metadata->set_oid(clovis_writer->get_oid());
  object_metadata->set_layout_id(layout_id);
  object_metadata->set_tags(new_object_tags_map);

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
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  }
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  probable_oid_list.clear();
  // store old object oid
  if (old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL) {
    std::string old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
    probable_oid_list[S3M0Uint128Helper::to_string(old_object_oid)] =
        object_metadata->create_probable_delete_record(old_layout_id);
  }

  // store new oid
  probable_oid_list[S3M0Uint128Helper::to_string(new_object_oid)] =
      object_metadata->create_probable_delete_record(layout_id);

  if (!clovis_kv_writer) {
    clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  clovis_kv_writer->put_keyval(
      global_probable_dead_object_list_index_oid, probable_oid_list,
      std::bind(&S3PutObjectAction::next, this),
      std::bind(
          &S3PutObjectAction::add_object_oid_to_probable_dead_oid_list_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Trigger rollback to undo changes done and report error
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (S3Option::get_instance()->is_getoid_enabled()) {

    request->set_out_header_value("x-stx-oid",
                                  S3M0Uint128Helper::to_string(new_object_oid));
    request->set_out_header_value("x-stx-layout-id", std::to_string(layout_id));
  }

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
    s3_timer.stop();
    const auto mss = s3_timer.elapsed_time_in_millisec();
    LOG_PERF("put_object_save_metadata_ms", request_id.c_str(), mss);
    s3_stats_timing("put_object_save_metadata", mss);
    // AWS adds explicit quotes "" to etag values.
    // https://docs.aws.amazon.com/AmazonS3/latest/API/API_PutObject.html
    std::string e_tag = "\"" + clovis_writer->get_content_md5() + "\"";

    request->set_out_header_value("ETag", e_tag);

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
  cleanup();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::cleanup() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (object_metadata &&
      (object_metadata->get_state() == S3ObjectMetadataState::saved)) {
    // process to delete old object
    if (clovis_writer &&
        (old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL)) {
      clovis_writer->set_oid(old_object_oid);
      clovis_writer->delete_object(
          std::bind(&S3PutObjectAction::cleanup_successful, this),
          std::bind(&S3PutObjectAction::cleanup_failed, this), old_layout_id);
    } else {
      cleanup_oid_from_probable_dead_oid_list();
    }
  } else {
    // metadata failed to delete, so remove the entry from
    // probable_dead_oid_list
    cleanup_oid_from_probable_dead_oid_list();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::cleanup_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::cleanup_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // new object oid and metadata saved but old object oid failed to delete
  // so erase the old object oid from probable_oid_list and
  // this old object oid will be retained in
  // global_probable_dead_object_list_index_oid
  probable_oid_list.erase(S3M0Uint128Helper::to_string(old_object_oid));
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::cleanup_oid_from_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::vector<std::string> clean_valid_oid_from_probable_dead_object_list_index;

  // final probable_oid_list map will have valid object oids
  // that needs to be cleanup from global_probable_dead_object_list_index_oid
  for (auto& kv : probable_oid_list) {
    clean_valid_oid_from_probable_dead_object_list_index.push_back(kv.first);
  }

  if (!clean_valid_oid_from_probable_dead_object_list_index.empty()) {
    if (!clovis_kv_writer) {
      clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
          request, s3_clovis_api);
    }
    clovis_kv_writer->delete_keyval(
        global_probable_dead_object_list_index_oid,
        clean_valid_oid_from_probable_dead_object_list_index,
        std::bind(&Action::done, this), std::bind(&Action::done, this));
  } else {
    done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
void S3PutObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

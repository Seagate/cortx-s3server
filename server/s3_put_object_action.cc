/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include "s3_put_object_action.h"
#include "s3_motr_layout.h"
#include "s3_common.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_uri_to_motr_oid.h"
#include <evhttp.h>
#include "s3_m0_uint128_helper.h"
#include "s3_perf_metrics.h"

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3PutObjectAction::S3PutObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_factory,
    std::shared_ptr<S3PutTagsBodyFactory> put_tags_body_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory)),
      total_data_to_stream(0),
      write_in_progress(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id, "S3 API: Put Object. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  action_uses_cleanup = true;
  s3_put_action_state = S3PutObjectActionState::empty;
  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  new_object_oid = {0ULL, 0ULL};

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }

  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);
  // Note valid value is set during create object
  layout_id = -1;

  tried_count = 0;
  salt = "uri_salt_";

  if (motr_s3_factory) {
    motr_writer_factory = std::move(motr_s3_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }
  if (put_tags_body_factory) {
    put_object_tag_body_factory = std::move(put_tags_body_factory);
  } else {
    put_object_tag_body_factory = std::make_shared<S3PutTagsBodyFactory>();
  }

  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  setup_steps();
}

void S3PutObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  if (!request->get_header_value("x-amz-tagging").empty()) {
    ACTION_TASK_ADD(S3PutObjectAction::validate_x_amz_tagging_if_present, this);
  }
  ACTION_TASK_ADD(S3PutObjectAction::validate_put_request, this);
  ACTION_TASK_ADD(S3PutObjectAction::create_object, this);
  ACTION_TASK_ADD(S3PutObjectAction::initiate_data_streaming, this);
  ACTION_TASK_ADD(S3PutObjectAction::save_metadata, this);
  ACTION_TASK_ADD(S3PutObjectAction::send_response_to_s3_client, this);
  // ...
}

void S3PutObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  s3_put_action_state = S3PutObjectActionState::validationFailed;
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
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::validate_put_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if ((request->get_object_name()).length() > MAX_OBJECT_KEY_LENGTH) {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("KeyTooLongError");
    send_response_to_s3_client();
  } else if (request->get_header_size() > MAX_HEADER_SIZE ||
             request->get_user_metadata_size() > MAX_USER_METADATA_SIZE) {

    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("BadRequest");
    send_response_to_s3_client();
  } else if (!request->is_header_present("Content-Length")) {
    // 'Content-Length' header is required and missing
    s3_log(S3_LOG_ERROR, request_id, "Missing mandatory Content-Length header");
    set_s3_error("MissingContentLength");
    send_response_to_s3_client();
  } else {
    next();
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
    s3_put_action_state = S3PutObjectActionState::validationFailed;
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
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::fetch_object_info_failed() {
  // Proceed to to next task, object metadata doesnt exist, will create now
  struct m0_uint128 object_list_oid =
      bucket_metadata->get_object_list_index_oid();
  if ((object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL) ||
      (objects_version_list_oid.u_hi == 0ULL &&
       objects_version_list_oid.u_lo == 0ULL)) {
    // Rare/unlikely: Motr KVS data corruption:
    // object_list_oid/objects_version_list_oid is null only when bucket
    // metadata is corrupted.
    // user has to delete and recreate the bucket again to make it work.
    s3_log(S3_LOG_ERROR, request_id, "Bucket(%s) metadata is corrupted.\n",
           request->get_bucket_name().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
    s3_put_action_state = S3PutObjectActionState::validationFailed;
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
    old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
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
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to look up metadata.\n");
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_timer.start();
  if (tried_count == 0) {
    motr_writer =
        motr_writer_factory->create_motr_writer(request, new_object_oid);
  } else {
    motr_writer->set_oid(new_object_oid);
  }

  layout_id = S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
      request->get_content_length());

  motr_writer->create_object(
      std::bind(&S3PutObjectAction::create_object_successful, this),
      std::bind(&S3PutObjectAction::create_object_failed, this), layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::newObjOidCreated;

  // New Object or overwrite, create new metadata and release old.
  new_object_metadata = object_metadata_factory->create_object_metadata_obj(
      request, bucket_metadata->get_object_list_index_oid());
  new_object_metadata->set_objects_version_list_index_oid(
      bucket_metadata->get_objects_version_list_index_oid());

  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  // Generate a version id for the new object.
  new_object_metadata->regenerate_version_id();
  new_object_metadata->set_oid(motr_writer->get_oid());
  new_object_metadata->set_layout_id(layout_id);

  add_object_oid_to_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::exists) {
    collision_detected();
  } else {
    s3_timer.stop();
    const auto mss = s3_timer.elapsed_time_in_millisec();
    LOG_PERF("create_object_failed_ms", request_id.c_str(), mss);
    s3_stats_timing("create_object_failed", mss);

    s3_put_action_state = S3PutObjectActionState::newObjOidCreationFailed;

    if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
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
    s3_put_action_state = S3PutObjectActionState::newObjOidCreationFailed;
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

    S3UriToMotrOID(s3_motr_api, salted_uri.c_str(), request_id,
                   &new_object_oid);

    ++salt_counter;
  } while ((new_object_oid.u_hi == current_oid.u_hi) &&
           (new_object_oid.u_lo == current_oid.u_lo));

  return;
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
          S3Option::get_instance()->get_motr_write_payload_size(layout_id));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_consume_incoming_content_shutdown_fail");
  if (request->is_s3_client_read_error()) {
    if (request->get_s3_client_read_error() == "RequestTimeout") {
      set_s3_error(request->get_s3_client_read_error());
      send_response_to_s3_client();
    }
    return;
  }

  log_timed_counter(put_timed_counter, "incoming_object_data_blocks");
  s3_perf_count_incoming_bytes(
      request->get_buffered_input()->get_content_length());
  // Resuming the action since we have data.
  if (!write_in_progress) {
    if (request->get_buffered_input()->is_freezed() ||
        request->get_buffered_input()->get_content_length() >=
            S3Option::get_instance()->get_motr_write_payload_size(layout_id)) {
      write_object(request->get_buffered_input());
    }
  }
  if (!request->get_buffered_input()->is_freezed() &&
      request->get_buffered_input()->get_content_length() >=
          (S3Option::get_instance()->get_motr_write_payload_size(layout_id) *
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

  motr_writer->write_content(
      std::bind(&S3PutObjectAction::write_object_successful, this),
      std::bind(&S3PutObjectAction::write_object_failed, this),
      std::move(buffer));

  write_in_progress = true;
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::write_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Write to motr successful\n");
  write_in_progress = false;

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (request->is_s3_client_read_error()) {
    client_read_error();
    return;
  }
  const bool is_memory_enough =
      mem_profile->we_have_enough_memory_for_put_obj(layout_id);

  if (!is_memory_enough) {
    s3_log(S3_LOG_ERROR, request_id, "Memory pool seems to be exhausted\n");
  }
  if (/* buffered data len is at least equal to max we can write to motr in
         one write */
      request->get_buffered_input()->get_content_length() >=
          S3Option::get_instance()->get_motr_write_payload_size(
              layout_id) || /* we have all the data buffered and ready to
                               write */
      (request->get_buffered_input()->is_freezed() &&
       request->get_buffered_input()->get_content_length() > 0)) {

    write_object(request->get_buffered_input());

    if (!is_memory_enough) {
      request->pause();
    } else if (!request->get_buffered_input()->is_freezed()) {
      // else we wait for more incoming data
      request->resume();
    }
  } else if (request->get_buffered_input()->is_freezed() &&
             request->get_buffered_input()->get_content_length() == 0) {
    // All data written to object
    s3_put_action_state = S3PutObjectActionState::writeComplete;
    next();
  } else if (!is_memory_enough) {
    set_s3_error("ServiceUnavailable");
    // Clean up will be done after response.
    // Treat write abandoned as write failure for further cleanups
    s3_put_action_state = S3PutObjectActionState::writeFailed;
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::write_object_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed writing to motr.\n");

  write_in_progress = false;
  s3_put_action_state = S3PutObjectActionState::writeFailed;

  if (request->is_s3_client_read_error()) {
    client_read_error();
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
    s3_log(S3_LOG_ERROR, request_id, "write_object_failed failure\n");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
}

void S3PutObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  std::string s_md5_got = request->get_header_value("content-md5");
  if (!s_md5_got.empty()) {
    std::string s_md5_calc = motr_writer->get_content_md5_base64();
    s3_log(S3_LOG_DEBUG, request_id, "MD5 calculated: %s, MD5 got %s",
           s_md5_calc.c_str(), s_md5_got.c_str());

    if (s_md5_calc != s_md5_got) {
      s3_log(S3_LOG_ERROR, request_id, "Content MD5 mismatch\n");
      s3_put_action_state = S3PutObjectActionState::md5ValidationFailed;

      set_s3_error("BadDigest");
      // Clean up will be done after response.
      send_response_to_s3_client();
      return;
    }
  }
  s3_timer.start();
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");

  new_object_metadata->reset_date_time_to_current();
  new_object_metadata->set_content_length(request->get_data_length_str());
  new_object_metadata->set_content_type(request->get_content_type());
  new_object_metadata->set_md5(motr_writer->get_content_md5());
  new_object_metadata->set_tags(new_object_tags_map);

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Writing user metadata on object: [%s] -> [%s]\n",
             it.first.c_str(), it.second.c_str());
      new_object_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }

  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  new_object_metadata->save(
      std::bind(&S3PutObjectAction::save_object_metadata_success, this),
      std::bind(&S3PutObjectAction::save_object_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::metadataSaved;
  next();
}

void S3PutObjectAction::save_object_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::metadataSaveFailed;
  if (new_object_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "failed to save object metadata.");
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi || old_object_oid.u_lo) {
    assert(!old_oid_str.empty());

    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding old_probable_del_rec with key [%s]\n",
           old_oid_rec_key.c_str());
    old_probable_del_rec.reset(new S3ProbableDeleteRecord(
        old_oid_rec_key, {0ULL, 0ULL}, object_metadata->get_object_name(),
        old_object_oid, old_layout_id,
        bucket_metadata->get_object_list_index_oid(),
        bucket_metadata->get_objects_version_list_index_oid(),
        object_metadata->get_version_key_in_index(), false /* force_delete */));

    probable_oid_list[old_oid_rec_key] = old_probable_del_rec->to_json();
  }

  s3_log(S3_LOG_DEBUG, request_id,
         "Adding new_probable_del_rec with key [%s]\n", new_oid_str.c_str());
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      new_oid_str, old_object_oid, new_object_metadata->get_object_name(),
      new_object_oid, layout_id, bucket_metadata->get_object_list_index_oid(),
      bucket_metadata->get_objects_version_list_index_oid(),
      new_object_metadata->get_version_key_in_index(),
      false /* force_delete */));

  // store new oid, key = newoid
  probable_oid_list[new_oid_str] = new_probable_del_rec->to_json();

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_oid, probable_oid_list,
      std::bind(&S3PutObjectAction::next, this),
      std::bind(
          &S3PutObjectAction::add_object_oid_to_probable_dead_oid_list_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
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
    // Metadata saved for object is always a success condition.
    assert(s3_put_action_state != S3PutObjectActionState::metadataSaved);

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
  } else {
    // Metadata saved implies its success.
    assert(s3_put_action_state == S3PutObjectActionState::metadataSaved);

    s3_put_action_state = S3PutObjectActionState::completed;

    s3_timer.stop();
    const auto mss = s3_timer.elapsed_time_in_millisec();
    LOG_PERF("put_object_save_metadata_ms", request_id.c_str(), mss);
    s3_stats_timing("put_object_save_metadata", mss);
    // AWS adds explicit quotes "" to etag values.
    // https://docs.aws.amazon.com/AmazonS3/latest/API/API_PutObject.html
    std::string e_tag = "\"" + motr_writer->get_content_md5() + "\"";

    request->set_out_header_value("ETag", e_tag);

    request->send_response(S3HttpSuccess200);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  request->resume(false);
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::startcleanup() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // TODO: Perf - all/some of below tasks can be done in parallel
  // Any of the following steps fail, backgrounddelete will be able to perform
  // cleanups.
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;

  // Success conditions
  if (s3_put_action_state == S3PutObjectActionState::completed) {
    s3_log(S3_LOG_DEBUG, request_id, "Cleanup old Object\n");
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // mark old OID for deletion in overwrite case, this optimizes
      // backgrounddelete decisions.
      ACTION_TASK_ADD(S3PutObjectAction::mark_old_oid_for_deletion, this);
    }
    // remove new oid from probable delete list.
    ACTION_TASK_ADD(S3PutObjectAction::remove_new_oid_probable_record, this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // Object overwrite case, old object exists, delete it.
      ACTION_TASK_ADD(S3PutObjectAction::delete_old_object, this);
      // If delete object is successful, attempt to delete old probable record
    }
  } else if (s3_put_action_state == S3PutObjectActionState::newObjOidCreated ||
             s3_put_action_state == S3PutObjectActionState::writeFailed ||
             s3_put_action_state ==
                 S3PutObjectActionState::md5ValidationFailed ||
             s3_put_action_state ==
                 S3PutObjectActionState::metadataSaveFailed) {
    // PUT is assumed to be failed with a need to rollback
    s3_log(S3_LOG_DEBUG, request_id,
           "Cleanup new Object: s3_put_action_state[%d]\n",
           s3_put_action_state);
    // Mark new OID for deletion, this optimizes backgrounddelete decisionss.
    ACTION_TASK_ADD(S3PutObjectAction::mark_new_oid_for_deletion, this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // remove old oid from probable delete list.
      ACTION_TASK_ADD(S3PutObjectAction::remove_old_oid_probable_record, this);
    }
    ACTION_TASK_ADD(S3PutObjectAction::delete_new_object, this);
    // If delete object is successful, attempt to delete new probable record
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "No Cleanup required: s3_put_action_state[%d]\n",
           s3_put_action_state);
    assert(s3_put_action_state == S3PutObjectActionState::empty ||
           s3_put_action_state == S3PutObjectActionState::validationFailed ||
           s3_put_action_state ==
               S3PutObjectActionState::probableEntryRecordFailed ||
           s3_put_action_state ==
               S3PutObjectActionState::newObjOidCreationFailed);
    // Nothing to undo
  }

  // Start running the cleanup task list
  start();
}

void S3PutObjectAction::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!new_oid_str.empty());

  // update new oid, key = newoid, force_del = true
  new_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             new_oid_str, new_probable_del_rec->to_json(),
                             std::bind(&S3PutObjectAction::next, this),
                             std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  // update old oid, force_del = true
  old_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             old_oid_rec_key, old_probable_del_rec->to_json(),
                             std::bind(&S3PutObjectAction::next, this),
                             std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                old_oid_rec_key,
                                std::bind(&S3PutObjectAction::next, this),
                                std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!new_oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                new_oid_str,
                                std::bind(&S3PutObjectAction::next, this),
                                std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::delete_old_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // If PUT is success, we delete old object if present
  assert(old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL);

  motr_writer->set_oid(old_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutObjectAction::remove_old_object_version_metadata, this),
      std::bind(&S3PutObjectAction::next, this), old_layout_id);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::remove_old_object_version_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  object_metadata->remove_version_metadata(
      std::bind(&S3PutObjectAction::remove_old_oid_probable_record, this),
      std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::delete_new_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // If PUT failed, then clean new object.
  assert(s3_put_action_state != S3PutObjectActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);

  motr_writer->set_oid(new_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutObjectAction::remove_new_oid_probable_record, this),
      std::bind(&S3PutObjectAction::next, this), layout_id);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

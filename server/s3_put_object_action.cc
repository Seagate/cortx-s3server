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
#include "s3_common_utilities.h"

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
      write_in_progress(false),
      last_object_size(0),
      primary_object_size(0),
      total_object_size_consumed(0),
      last_fragment_calc_size(0),
      current_fault_iteration(0),
      fault_mode_active(false),
      create_fragment_when_write_success(false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Put Object. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  action_uses_cleanup = true;
  s3_put_action_state = S3PutObjectActionState::empty;
  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  new_object_oid = {0ULL, 0ULL};
  no_of_blocks_written = 0;

  // Default to 10 objects in S3 fault mode
  max_objects_in_s3_fault_mode = MAX_ALLOWED_RECOVERY_IN_FAULT_MODE;
  if (S3Option::get_instance()->get_max_objects_in_fault_mode() > 0) {
    max_objects_in_s3_fault_mode =
        S3Option::get_instance()->get_max_objects_in_fault_mode();
    if (max_objects_in_s3_fault_mode > MAX_ALLOWED_RECOVERY_IN_FAULT_MODE) {
      // Restrict recovery attempts to system defined
      max_objects_in_s3_fault_mode = MAX_ALLOWED_RECOVERY_IN_FAULT_MODE;
    }
  } else {
    // S3 fault mode is disabled
    max_objects_in_s3_fault_mode = 0;
  }
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
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::validate_x_amz_tagging_if_present() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  struct evkeyvalq key_value;
  memset(&key_value, 0, sizeof(key_value));
  if (0 == evhttp_parse_query_str(content.c_str(), &key_value)) {
    char* decoded_key = NULL;
    for (struct evkeyval* header = key_value.tqh_first; header;
         header = header->next.tqe_next) {

      decoded_key = evhttp_uridecode(header->key, 0, NULL);
      s3_log(S3_LOG_DEBUG, request_id,
             "Successfully parsed the Key Values=%s %s", decoded_key,
             header->value);
      new_object_tags_map[decoded_key] = header->value;
      free(decoded_key);
      decoded_key = NULL;
    }
    validate_tags();
  } else {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::validate_tags() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
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
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::_set_layout_id(int layout_id) {
  assert(layout_id > 0 && layout_id < 15);
  this->layout_id = layout_id;

  motr_write_payload_size =
      S3Option::get_instance()->get_motr_write_payload_size(layout_id);
}

void S3PutObjectAction::create_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_timer.start();

  if (tried_count == 0) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }
  if (!this->fault_mode_active) {
    // S3 non-fault(normal) mode
    // Save primary object size, if further object write fails,
    // thereby creating another object.
    primary_object_size = request->get_content_length();
    _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
        primary_object_size));
  } else {
    // S3 fault mode is active
    // The actual fragment size might be less than what is set here, if there
    // is a further write fault, creating another object.
    last_fragment_calc_size =
        request->get_content_length() - this->total_object_size_consumed;
    _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
        last_fragment_calc_size));
  }
  motr_writer->create_object(
      std::bind(&S3PutObjectAction::create_object_successful, this),
      std::bind(&S3PutObjectAction::create_object_failed, this), new_object_oid,
      layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::create_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::newObjOidCreated;
  new_obj_oids.push_back(motr_writer->get_oid());

  // Reset the size of last object/fragment
  last_object_size = 0;

  if (!this->fault_mode_active) {
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
  } else {
    // Create extended metadata object, with parts=0, fragments=0
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_object_metadata =
        new_object_metadata->get_extended_object_metadata();
    if (!ext_object_metadata) {
      std::shared_ptr<S3ObjectExtendedMetadata> new_ext_object_metadata =
          object_metadata_factory->create_object_ext_metadata_obj(
              request, request->get_bucket_name(), request->get_object_name(),
              new_object_metadata->get_obj_version_key(),
              bucket_metadata->get_extended_metadata_index_oid());
      new_object_metadata->set_extended_object_metadata(
          new_ext_object_metadata);
      s3_log(S3_LOG_DEBUG, stripped_request_id,
             "Created extended object metadata");
    }
    // Add extended object's current fragment to probable delete list
    size_t extended_obj_size =
        request->get_content_length() - total_object_size_consumed;
    unsigned int layoutid =
        S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
            extended_obj_size);
    add_extended_object_oid_to_probable_dead_oid_list(
        motr_writer->get_oid(), layoutid, extended_obj_size,
        new_object_metadata, new_ext_probable_del_rec_list);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::create_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
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
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  if (tried_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid(new_object_oid);
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, stripped_request_id,
             "Object ID collision happened %d times for uri %s\n", tried_count,
             request->get_object_uri().c_str());
    }
    create_object();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Exceeded maximum collision retry attempts."
           "Collision occurred %d times for uri %s\n",
           tried_count, request->get_object_uri().c_str());
    // s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
    //     S3_IEM_COLLISION_RES_FAIL_JSON);
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
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
          motr_write_payload_size);
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
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
            motr_write_payload_size) {
      write_object(request->get_buffered_input());
    }
  }
  if (!request->get_buffered_input()->is_freezed() &&
      request->get_buffered_input()->get_content_length() >=
          (motr_write_payload_size *
           S3Option::get_instance()->get_read_ahead_multiple())) {
    s3_log(S3_LOG_DEBUG, request_id, "Pausing with Buffered length = %zu\n",
           request->get_buffered_input()->get_content_length());
    request->pause();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::write_object(
    std::shared_ptr<S3AsyncBufferOptContainer> buffer) {

  size_t content_length = buffer->get_content_length();
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry with buffer length = %zu\n",
         __func__, content_length);

  if (content_length > motr_write_payload_size) {
    content_length = motr_write_payload_size;
  }
  last_buffer_seq = buffer->get_buffers(content_length);
  motr_writer->write_content(
      std::bind(&S3PutObjectAction::write_object_successful, this),
      std::bind(&S3PutObjectAction::write_object_failed, this), last_buffer_seq,
      buffer->size_of_each_evbuf);

  write_in_progress = true;
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::write_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id, "Write to motr successful\n");

  write_in_progress = false;
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  if (request->is_s3_client_read_error()) {
    client_read_error();
    return;
  }

  request->get_buffered_input()->flush_used_buffers();

  no_of_blocks_written++;
  last_object_size = motr_writer->get_size_of_data_written();

  motr_writer->set_buffer_rewrite_flag(false);

  if (fault_mode_active && create_fragment_when_write_success) {
    create_fragment_when_write_success = false;
    // This is write success for the fragment
    // Add entry to the extended object
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_object_metadata =
        new_object_metadata->get_extended_object_metadata();
    if (ext_object_metadata) {
      struct m0_uint128 extended_oid = motr_writer->get_oid();
      struct s3_part_frag_context part_frag_ctx;
      part_frag_ctx.motr_OID = extended_oid;
      // Set null PVID
      part_frag_ctx.PVID =
          S3M0Uint128Helper::to_m0_uint128("AAAAAAAAAAA=-AAAAAAAAAAA=");
      part_frag_ctx.versionID = new_object_metadata->get_obj_version_key();
      // Set the initial calculated size of fragment. The size may change
      // if there is a fault during data write to this fragment.
      // The effective size is set later, if another faults happens.
      part_frag_ctx.item_size = last_fragment_calc_size;
      part_frag_ctx.layout_id = layout_id;
      part_frag_ctx.is_multipart = false;
      // For simple (non-multipart) object, set part_no=0
      ext_object_metadata->add_extended_entry(part_frag_ctx,
                                              current_fault_iteration, 0);
      new_object_metadata->set_number_of_fragments(
          ext_object_metadata->get_fragment_count());
      s3_log(S3_LOG_INFO, request_id,
             "Added extended entry for object oid: "
             "%" SCNx64 " : %" SCNx64,
             extended_oid.u_hi, extended_oid.u_lo);
    }
  }

  const bool is_memory_enough =
      mem_profile->we_have_enough_memory_for_put_obj(layout_id);

  if (!is_memory_enough) {
    s3_log(S3_LOG_ERROR, request_id, "Memory pool seems to be exhausted\n");
  }
  if (/* buffered data len is at least equal to max we can write to motr in
         one write */
      request->get_buffered_input()->get_content_length() >=
          motr_write_payload_size ||
      // we have all the data buffered and ready to write
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::write_object_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed writing to motr.\n");

  if (request->is_s3_client_read_error()) {
    client_read_error();
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
    s3_put_action_state = S3PutObjectActionState::writeFailed;
    write_in_progress = false;
    request->get_buffered_input()->flush_used_buffers();
    s3_log(S3_LOG_ERROR, request_id, "write_object_failed failure\n");
    send_response_to_s3_client();
  } else {
    // Determine here further whether it is due to Motr degradation mode or
    // irrecoverable CLOVIS error (in which case we simply return
    // "InternalError").
    // TODO: Check if this failure is due to Motr degradation, causing write
    // object
    // failure. Need to check specific error code/state from Motr.
    // Work-around until we get Motr changes for new error code:
    //  Check fault tolerence object creation count/threshhold. If it is > 0
    //  fault tolerence mode is enabled. We'll support multiple object creation
    //  upto the threshold.
    //  Presently, the below code to handle Motr degradation mode is executed
    //  using S3 FI in write object operation and the enablement of feature in
    //  S3 config file using "S3_MAX_EXTENDED_OBJECTS_IN_FAULT_MODE" > 0.
    if (max_objects_in_s3_fault_mode &&
        (current_fault_iteration++ < max_objects_in_s3_fault_mode)) {
      s3_log(S3_LOG_INFO, request_id,
             "Creating new object and writing data to it...\n");
      // S3 fault mode is enabled. Activate fault mode
      if (!fault_mode_active) {
        fault_mode_active = true;
        // Set object with only fragments
        new_object_metadata->set_object_type(
            S3ObjectMetadataType::only_frgments);
      }
      total_object_size_consumed += last_object_size;
      // TBD:Save current object oid (stored in new_object_oid), before creating
      // new OID.
      S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                     &new_object_oid);
      // TBD: Before creating next fragment, re-calculate layout id
      // of current fragment.
      if (current_fault_iteration == 1) {
        // Save the size of primary object
        primary_object_size = last_object_size;
        s3_log(S3_LOG_DEBUG, request_id, "Primary object size = [%zu]",
               last_object_size);
        new_object_metadata->set_primary_obj_size(primary_object_size);
      } else {
        // Save size of next fragment
        const std::shared_ptr<S3ObjectExtendedMetadata>& ext_object_metadata =
            new_object_metadata->get_extended_object_metadata();
        if (ext_object_metadata) {
          // Set size of the next fragment
          ext_object_metadata->set_size_of_extended_entry(
              last_object_size, (current_fault_iteration - 1), 0);
          s3_log(S3_LOG_DEBUG, request_id, "Fragment size = [%zu]",
                 last_object_size);
        }
      }
      tried_count = 0;
      // Set flag below to stop getting read-data-available callback,
      // until the fragmented object is created.
      write_in_progress = true;
      // Save MD5 digest hash instance
      last_MD5Hash_state = motr_writer->get_MD5Hash_instance();
      // Create next object(fragment)
      create_object();
      // TBD: Create another object
      // In success of object creation, if fault_mode_active is true,
      // create extended object metadata and add object oid to it.
      return;
    } else {
      // This is a general write object failure
      s3_put_action_state = S3PutObjectActionState::writeFailed;
      write_in_progress = false;
      request->get_buffered_input()->flush_used_buffers();
      set_s3_error("InternalError");
      // Clean up will be done after response.
      send_response_to_s3_client();
    }
  }
}

void S3PutObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  std::string s_md5_got = request->get_header_value("content-md5");
  if (!s_md5_got.empty() && !motr_writer->content_md5_matches(s_md5_got)) {
    s3_log(S3_LOG_ERROR, request_id, "Content MD5 mismatch\n");
    s3_put_action_state = S3PutObjectActionState::md5ValidationFailed;

    set_s3_error("BadDigest");
    // Clean up will be done after response.
    send_response_to_s3_client();
    return;
  }
  s3_timer.start();
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");

  new_object_metadata->reset_date_time_to_current();
  new_object_metadata->set_content_length(request->get_data_length_str());
  new_object_metadata->set_content_type(request->get_content_type());
  new_object_metadata->set_md5(motr_writer->get_content_md5());
  new_object_metadata->set_tags(new_object_tags_map);
  new_object_metadata->set_pvid(motr_writer->get_ppvid());

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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::metadataSaved;
  next();
}

void S3PutObjectAction::save_object_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::add_extended_object_oid_to_probable_dead_oid_list(
    struct m0_uint128 extended_oid, unsigned int layout_id,
    const size_t& extended_object_size,
    const std::shared_ptr<S3ObjectMetadata>& object_metadata,
    std::vector<std::unique_ptr<S3ProbableDeleteRecord>>&
        probable_del_rec_list) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  // Check not null extended_oid and prroceed
  if (!extended_oid.u_hi && !extended_oid.u_lo) {
    return;
  }
  struct m0_uint128 null_oid = {0ULL, 0ULL};
  std::string extended_obj_oid_str = S3M0Uint128Helper::to_string(extended_oid);

  // prepending a char depending on the size of the object (size based
  // bucketing of object)
  S3CommonUtilities::size_based_bucketing_of_objects(extended_obj_oid_str,
                                                     extended_object_size);
  std::unique_ptr<S3ProbableDeleteRecord> probable_del_rec;
  s3_log(S3_LOG_DEBUG, request_id,
         "Adding extended probable del rec with key [%s]\n",
         extended_obj_oid_str.c_str());
  probable_del_rec.reset(new S3ProbableDeleteRecord(
      extended_obj_oid_str, null_oid, object_metadata->get_object_name(),
      extended_oid, layout_id, bucket_metadata->get_object_list_index_oid(),
      bucket_metadata->get_objects_version_list_index_oid(),
      object_metadata->get_version_key_in_index(), false /* force_delete */));

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_oid, extended_obj_oid_str,
      probable_del_rec->to_json(),
      std::bind(&S3PutObjectAction::continue_object_write, this),
      std::bind(&S3PutObjectAction::
                     add_extended_object_oid_to_probable_dead_oid_list_failed,
                this));
  probable_del_rec_list.push_back(std::move(probable_del_rec));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

#if 0
//  TODO: Do we need this function?  
void S3PutObjectAction::add_extended_object_oids_to_probable_dead_oid_list(
    std::vector<struct m0_uint128> extended_oids,
    std::vector<unsigned int> layout_ids,
    std::vector<size_t> extended_object_sizes,
    const std::shared_ptr<S3ObjectMetadata>& object_metadata,
    std::vector<std::unique_ptr<S3ProbableDeleteRecord>>&
        probable_del_rec_list) {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  if (extended_oids.size() != layout_ids.size() ||
      layout_ids.size() != extended_object_sizes.size()) {
    return;
  }
  // Check not null extended_oid and proceed
  for (auto& oid : extended_oids) {
    if (!oid.u_hi && !oid.u_lo) {
      return;
    }
  }
  struct m0_uint128 null_oid = {0ULL, 0ULL};
  int idx = 0;
  std::map<std::string, std::string> kv_mp;

  for (auto& extended_oid : extended_oids) {
    std::string extended_obj_oid_str =
        S3M0Uint128Helper::to_string(extended_oid);

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(
        extended_obj_oid_str, extended_object_sizes[idx]);
    std::unique_ptr<S3ProbableDeleteRecord> probable_del_rec;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding extended probable del rec with key [%s]\n",
           extended_obj_oid_str.c_str());
    probable_del_rec.reset(new S3ProbableDeleteRecord(
        extended_obj_oid_str, null_oid, object_metadata->get_object_name(),
        extended_oid, layout_ids[idx],
        bucket_metadata->get_object_list_index_oid(),
        bucket_metadata->get_objects_version_list_index_oid(),
        object_metadata->get_version_key_in_index(), false /* force_delete */));
    kv_mp[probable_del_rec->get_key()] = probable_del_rec->to_json();
    idx++;
    probable_del_rec_list.push_back(std::move(probable_del_rec));
  }

  if (idx > 0) {
    if (!motr_kv_writer) {
      motr_kv_writer =
          mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
    }
    motr_kv_writer->put_keyval(
        global_probable_dead_object_list_index_oid, kv_mp,
        std::bind(&S3PutObjectAction::continue_object_write, this),
        std::bind(&S3PutObjectAction::
                       add_extended_object_oid_to_probable_dead_oid_list_failed,
                  this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}
#endif

void S3PutObjectAction::continue_object_write() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Set flag in IO write to indicate re-write of same buffer,
  // so that MD5 disgest is not re-caulculated on same buffer.
  motr_writer->set_buffer_rewrite_flag(true);
  // Re-set the last MD5 hash context
  motr_writer->set_MD5Hash_instance(last_MD5Hash_state);
  create_fragment_when_write_success = true;
  // Re-write the same buffer to newly created Motr object
  motr_writer->write_content(
      std::bind(&S3PutObjectAction::write_object_successful, this),
      std::bind(&S3PutObjectAction::write_object_failed, this), last_buffer_seq,
      (request->get_buffered_input())->size_of_each_evbuf);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void
S3PutObjectAction::add_extended_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Extended entr(y/ies) will be removed during Clean-up stage.
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi || old_object_oid.u_lo) {
    assert(!old_oid_str.empty());

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    size_t object_size = object_metadata->get_content_length();
    if (object_metadata->is_object_extended()) {
      // If object is extended, the primary object size will be in "Size" field
      object_size = object_metadata->get_primary_obj_size();
    }
    S3CommonUtilities::size_based_bucketing_of_objects(old_oid_str,
                                                       object_size);

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
    old_ext_probable_del_rec_list.push_back(std::move(old_probable_del_rec));
    old_obj_oids.push_back(old_object_oid);
    if (object_metadata->is_object_extended()) {
      // If old object is fragmented, add extended object oids
      // to probable delete list
      const std::shared_ptr<S3ObjectExtendedMetadata>& ext_old_metadata =
          object_metadata->get_extended_object_metadata();
      if (ext_old_metadata->has_entries()) {
        const std::map<int, std::vector<struct s3_part_frag_context>>&
            ext_entries_mp = ext_old_metadata->get_raw_extended_entries();
        const std::vector<struct s3_part_frag_context>& ext_entries =
            ext_entries_mp.at(0);
        for (const auto& ext_entry : ext_entries) {
          if (ext_entry.motr_OID.u_hi || ext_entry.motr_OID.u_lo) {
            std::string ext_old_oid_str =
                S3M0Uint128Helper::to_string(ext_entry.motr_OID);
            // prepending a char depending on the size of the object (size based
            // bucketing of object)
            S3CommonUtilities::size_based_bucketing_of_objects(
                ext_old_oid_str, ext_entry.item_size);

            // key = oldoid + "-" + newoid
            // std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
            s3_log(S3_LOG_DEBUG, request_id,
                   "Adding old extended oid with key [%s] to probable delete "
                   "record\n",
                   ext_old_oid_str.c_str());

            std::unique_ptr<S3ProbableDeleteRecord> ext_old_del_rec;
            ext_old_del_rec.reset(new S3ProbableDeleteRecord(
                ext_old_oid_str, {0ULL, 0ULL},
                object_metadata->get_object_name(), ext_entry.motr_OID,
                ext_entry.layout_id,
                bucket_metadata->get_object_list_index_oid(),
                bucket_metadata->get_objects_version_list_index_oid(),
                object_metadata->get_version_key_in_index(),
                false /* force_delete */));

            probable_oid_list[ext_old_del_rec->get_key()] =
                ext_old_del_rec->to_json();
            old_ext_probable_del_rec_list.push_back(std::move(ext_old_del_rec));
            old_obj_oids.push_back(ext_entry.motr_OID);
          }
        }
      }
    }
  }

  // prepending a char depending on the size of the object (size based bucketing
  // of object)
  S3CommonUtilities::size_based_bucketing_of_objects(
      new_oid_str, request->get_content_length());

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
  new_ext_probable_del_rec_list.push_back(std::move(new_probable_del_rec));

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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::startcleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
    if (new_object_metadata && !new_object_metadata->is_object_extended()) {
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
    } else {
      // Object is extended/fragmented. If there were any objects created
      // and written to, before getting 'probableEntryRecordFailed' or
      // 'newObjOidCreationFailed', then we need to delete those objects.
      // So, add a task to delete such objects.
      if (s3_put_action_state ==
              S3PutObjectActionState::probableEntryRecordFailed ||
          s3_put_action_state ==
              S3PutObjectActionState::newObjOidCreationFailed) {
        ACTION_TASK_ADD(S3PutObjectAction::mark_new_oid_for_deletion, this);
        if (old_object_oid.u_hi || old_object_oid.u_lo) {
          // remove old oid from probable delete list.
          ACTION_TASK_ADD(S3PutObjectAction::remove_old_oid_probable_record,
                          this);
        }
        ACTION_TASK_ADD(S3PutObjectAction::delete_new_object, this);
      }
    }
  }

  // Start running the cleanup task list
  start();
}

void S3PutObjectAction::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (new_ext_probable_del_rec_list.size() == 0) {
    return;
  }
  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : new_ext_probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    oid_key_val_mp[delete_rec->get_key()] = delete_rec->to_json();
  }
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             oid_key_val_mp,
                             std::bind(&S3PutObjectAction::next, this),
                             std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  if (old_ext_probable_del_rec_list.size() == 0) {
    return;
  }
  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : old_ext_probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    std::string prepended_new_oid_str = new_oid_str;
    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = delete_rec->get_key();
    if (old_oid_rec_key.find("-") != std::string::npos) {
      old_oid_rec_key = old_oid_str + '-' + prepended_new_oid_str.erase(0, 1);
    }
    oid_key_val_mp[old_oid_rec_key] = delete_rec->to_json();
  }
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             oid_key_val_mp,
                             std::bind(&S3PutObjectAction::next, this),
                             std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::mark_new_extended_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (new_ext_probable_del_rec_list.size() == 0) {
    return;
  }
  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : new_ext_probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    oid_key_val_mp[delete_rec->get_key()] = delete_rec->to_json();
  }
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             oid_key_val_mp,
                             std::bind(&S3PutObjectAction::next, this),
                             std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());
  if (old_ext_probable_del_rec_list.size() == 0) {
    return;
  }
  std::vector<std::string> keys_to_delete;
  for (auto& probable_rec : old_ext_probable_del_rec_list) {
    // key = oldoid + "-" + newoid
    std::string prepended_new_oid_str = new_oid_str;
    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = probable_rec->get_key();
    if (old_oid_rec_key.find("-") != std::string::npos) {
      old_oid_rec_key = old_oid_str + '-' + prepended_new_oid_str.erase(0, 1);
    }
    keys_to_delete.push_back(old_oid_rec_key);
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                keys_to_delete,
                                std::bind(&S3PutObjectAction::next, this),
                                std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());
  if (new_ext_probable_del_rec_list.size() == 0) {
    return;
  }
  std::vector<std::string> keys_to_delete;
  for (auto& probable_rec : new_ext_probable_del_rec_list) {
    keys_to_delete.push_back(probable_rec->get_key());
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                keys_to_delete,
                                std::bind(&S3PutObjectAction::next, this),
                                std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::delete_old_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT is success, we delete old object if present
  assert(old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL);

  // If old object exists and deletion of old is disabled, then return
  if ((old_object_oid.u_hi || old_object_oid.u_lo) &&
      S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    s3_log(S3_LOG_INFO, request_id,
           "Skipping deletion of old object. The old object will be deleted by "
           "BD.\n");
    // Call next task in the pipeline
    next();
    return;
  }
  unsigned int object_count = old_obj_oids.size();
  if (object_count > 0) {
    motr_writer->delete_object(
        std::bind(&S3PutObjectAction::delete_old_object_success, this),
        std::bind(&S3PutObjectAction::next, this), old_obj_oids.back(),
        layout_id);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::delete_old_object_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  unsigned int object_count = old_obj_oids.size();
  if (object_count == 0) {
    return;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Deleted old object oid "
         "[%" SCNx64 " : %" SCNx64 "]",
         (old_obj_oids.back()).u_hi, (old_obj_oids.back()).u_lo);
  old_obj_oids.pop_back();

  if (old_obj_oids.size() > 0) {
    delete_old_object();
  } else {
    remove_old_object_version_metadata();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::remove_old_object_version_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  object_metadata->remove_version_metadata(
      std::bind(&S3PutObjectAction::remove_old_oid_probable_record, this),
      std::bind(&S3PutObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::delete_new_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT failed, then clean new object.
  assert(s3_put_action_state != S3PutObjectActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);
  unsigned int object_count = new_obj_oids.size();
  if (object_count > 0) {
    motr_writer->delete_object(
        std::bind(&S3PutObjectAction::delete_new_object_success, this),
        std::bind(&S3PutObjectAction::next, this), new_obj_oids.back(),
        layout_id);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::delete_new_object_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  unsigned int object_count = new_obj_oids.size();
  if (object_count == 0) {
    return;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Deleted new object oid "
         "[%" SCNx64 " : %" SCNx64 "]",
         (new_obj_oids.back()).u_hi, (new_obj_oids.back()).u_lo);
  new_obj_oids.pop_back();

  if (new_obj_oids.size() > 0) {
    delete_new_object();
  } else {
    remove_new_oid_probable_record();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

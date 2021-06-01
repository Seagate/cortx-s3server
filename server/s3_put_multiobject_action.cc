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

#include "s3_put_multiobject_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_perf_metrics.h"
#include "s3_stats.h"
#include "s3_m0_uint128_helper.h"
#include "s3_common_utilities.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_common_utilities.h"

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3PutMultiObjectAction::S3PutMultiObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<S3AuthClientFactory> auth_factory)
    : S3ObjectAction(std::move(req), nullptr, nullptr, true,
                     std::move(auth_factory)),
      total_data_to_stream(0),
      auth_failed(false),
      write_failed(false),
      motr_write_in_progress(false),
      motr_write_completed(false),
      auth_in_progress(false),
      auth_completed(false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  part_number = get_part_number();
  upload_id = request->get_query_string_value("uploadId");

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Upload Part. Bucket[%s] Object[%s]\
         Part[%d] for UploadId[%s]\n",
         request->get_bucket_name().c_str(), request->get_object_name().c_str(),
         part_number, upload_id.c_str());

  action_uses_cleanup = true;
  s3_put_action_state = S3PutPartActionState::empty;
  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  layout_id = -1;  // Will be set during create object
  new_object_oid = {0ULL, 0ULL};

  if (S3Option::get_instance()->is_auth_disabled()) {
    auth_completed = true;
  }

  if (object_mp_meta_factory) {
    object_mp_metadata_factory = object_mp_meta_factory;
  } else {
    object_mp_metadata_factory =
        std::make_shared<S3ObjectMultipartMetadataFactory>();
  }

  if (part_meta_factory) {
    part_metadata_factory = part_meta_factory;
  } else {
    part_metadata_factory = std::make_shared<S3PartMetadataFactory>();
  }

  if (motr_s3_writer_factory) {
    motr_writer_factory = motr_s3_writer_factory;
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }

  if (kv_writer_factory) {
    motr_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    motr_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);

  setup_steps();
}

void S3PutMultiObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  ACTION_TASK_ADD(S3PutMultiObjectAction::validate_multipart_request, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::check_part_details, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::fetch_multipart_metadata, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::fetch_part_info, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::create_part_object, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::initiate_data_streaming, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::save_metadata, this);
  ACTION_TASK_ADD(S3PutMultiObjectAction::send_response_to_s3_client, this);
  // ...
}

void S3PutMultiObjectAction::validate_multipart_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (!request->is_chunked() && !request->is_header_present("Content-Length")) {
    // For non-chunked upload, the Header 'Content-Length' is missing
    s3_log(S3_LOG_INFO, stripped_request_id, "Missing Content-Length header");
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("MissingContentLength");
    send_response_to_s3_client();
  } else {
    next();
  }
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit", __func__);
}

void S3PutMultiObjectAction::check_part_details() {
  // "Part numbers can be any number from 1 to 10,000, inclusive."
  // https://docs.aws.amazon.com/en_us/AmazonS3/latest/API/API_UploadPart.html
  if (part_number < MINIMUM_PART_NUMBER || part_number > MAXIMUM_PART_NUMBER) {
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("InvalidPart");
    send_response_to_s3_client();
  } else if (request->get_header_size() > MAX_HEADER_SIZE ||
             request->get_user_metadata_size() > MAX_USER_METADATA_SIZE) {
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("MetadataTooLarge");
    send_response_to_s3_client();
  } else if ((request->get_object_name()).length() > MAX_OBJECT_KEY_LENGTH) {
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("KeyTooLongError");
    send_response_to_s3_client();
  } else if (request->get_content_length() > MAXIMUM_ALLOWED_PART_SIZE) {
    set_s3_error("EntityTooLarge");
    send_response_to_s3_client();
  } else {
    next();
  }
}

void S3PutMultiObjectAction::chunk_auth_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  auth_in_progress = false;
  auth_completed = true;
  if (check_shutdown_and_rollback(true)) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  if (motr_write_completed) {
    if (write_failed) {
      // No need of setting error as its set when we set write_failed
      send_response_to_s3_client();
    } else {
      next();
    }
  } else {
    // wait for write to complete. do nothing here.
  }
}

void S3PutMultiObjectAction::chunk_auth_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  auth_failed = true;
  auth_completed = true;
  set_s3_error("SignatureDoesNotMatch");
  if (check_shutdown_and_rollback(true)) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  if (motr_write_in_progress) {
    // Do nothing, handle after write returns
  } else {
    send_response_to_s3_client();
  }
}

void S3PutMultiObjectAction::fetch_bucket_info_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    next();
  } else {
    set_s3_error("NoSuchBucket");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::fetch_object_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "Entering\n");
  auto omds = object_metadata->get_state();
  if (omds == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
    next();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Metadata load state %d\n", (int)omds);
    if (omds == S3ObjectMetadataState::failed_to_launch) {
      set_s3_error("ServiceUnavailable");
    } else {
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, stripped_request_id, "Exiting\n");
}

void S3PutMultiObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Bucket does not exists\n");
  s3_put_action_state = S3PutPartActionState::validationFailed;
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void S3PutMultiObjectAction::fetch_multipart_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string multipart_key_name =
      request->get_object_name() + EXTENDED_METADATA_OBJECT_SEP + upload_id;
  object_multipart_metadata =
      object_mp_metadata_factory->create_object_mp_metadata_obj(
          request, bucket_metadata->get_multipart_index_oid(), upload_id);

  // In multipart table key is object_name + |uploadid
  object_multipart_metadata->rename_object_name(multipart_key_name);

  object_multipart_metadata->load(
      std::bind(&S3PutMultiObjectAction::next, this),
      std::bind(&S3PutMultiObjectAction::fetch_multipart_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::fetch_multipart_failed() {
  // Log error
  s3_put_action_state = S3PutPartActionState::validationFailed;
  s3_log(S3_LOG_ERROR, request_id,
         "Failed to retrieve multipart upload metadata\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::missing) {
    set_s3_error("NoSuchUpload");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void S3PutMultiObjectAction::fetch_part_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  part_metadata = part_metadata_factory->create_part_metadata_obj(
      request, object_multipart_metadata->get_part_index_oid(), upload_id,
      part_number);

  part_metadata->load(
      std::bind(&S3PutMultiObjectAction::fetch_part_info_success, this),
      std::bind(&S3PutMultiObjectAction::fetch_part_info_failed, this),
      part_number);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::fetch_part_info_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata->get_state() == S3PartMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id,
           "S3PartMetadataState::present for part %d\n", part_number);
    old_object_oid = part_metadata->get_oid();
    old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
    old_layout_id = part_metadata->get_layout_id();
    next();
  } else if (part_metadata->get_state() == S3PartMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "S3PartMetadataState::missing\n");
    next();
  } else if (part_metadata->get_state() ==
             S3PartMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Part metadata load operation failed due to pre launch failure\n");
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to look up metadata.\n");
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::fetch_part_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata->get_state() == S3PartMetadataState::missing) {
    next();
  } else {
    s3_put_action_state = S3PutPartActionState::validationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::_set_layout_id(int layout_id) {
  assert(layout_id > 0 && layout_id < 15);
  this->layout_id = layout_id;

  motr_write_payload_size =
      S3Option::get_instance()->get_motr_write_payload_size(layout_id);
}

void S3PutMultiObjectAction::create_part_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_timer.start();
  motr_writer = motr_writer_factory->create_motr_writer(
      request, new_object_oid, object_multipart_metadata->get_pvid(), 0);
  _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
      request->get_content_length()));
  motr_writer->set_layout_id(layout_id);

  motr_writer->create_object(
      std::bind(&S3PutMultiObjectAction::create_part_object_successful, this),
      std::bind(&S3PutMultiObjectAction::create_part_object_failed, this),
      new_object_oid, layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_multiobject_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::create_part_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutPartActionState::newObjOidCreated;

  part_metadata = part_metadata_factory->create_part_metadata_obj(
      request, object_multipart_metadata->get_part_index_oid(), upload_id,
      part_number);
  part_metadata->set_oid(motr_writer->get_oid());
  part_metadata->set_layout_id(layout_id);
  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  add_object_oid_to_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::create_part_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  s3_timer.stop();
  const auto mss = s3_timer.elapsed_time_in_millisec();
  LOG_PERF("create_object_failed_ms", request_id.c_str(), mss);
  s3_stats_timing("create_object_failed", mss);

  s3_put_action_state = S3PutPartActionState::newObjOidCreationFailed;

  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Create object failed.\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
    // Any other error report failure.
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  total_data_to_stream = request->get_data_length();
  // request->resume();

  if (request->is_chunked() && !S3Option::get_instance()->is_auth_disabled()) {
    get_auth_client()->init_chunk_auth_cycle(
        std::bind(&S3PutMultiObjectAction::chunk_auth_successful, this),
        std::bind(&S3PutMultiObjectAction::chunk_auth_failed, this));
  }

  if (total_data_to_stream == 0) {
    next();  // Zero size object.
  } else {
    if (request->has_all_body_content()) {
      write_object(request->get_buffered_input());
    } else {
      s3_log(S3_LOG_DEBUG, request_id,
             "We do not have all the data, so start listening....\n");
      // Start streaming, logically pausing action till we get data.
      request->listen_for_incoming_data(
          std::bind(&S3PutMultiObjectAction::consume_incoming_content, this),
          motr_write_payload_size);
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::consume_incoming_content() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_multiobject_action_consume_incoming_content_shutdown_fail");
  if (request->is_s3_client_read_error()) {
    if (!motr_write_in_progress) {
      client_read_error();
    }
    return;
  }

  log_timed_counter(put_timed_counter, "incoming_object_data_blocks");
  s3_perf_count_incoming_bytes(
      request->get_buffered_input()->get_content_length());
  if (!motr_write_in_progress) {
    if (request->get_buffered_input()->is_freezed() ||
        request->get_buffered_input()->get_content_length() >=
            motr_write_payload_size) {
      write_object(request->get_buffered_input());
      if (!motr_write_in_progress && motr_write_completed) {
        s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
        return;
      }
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

void S3PutMultiObjectAction::send_chunk_details_if_any() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  while (request->is_chunk_detail_ready()) {
    S3ChunkDetail detail = request->pop_chunk_detail();
    s3_log(S3_LOG_DEBUG, request_id, "Using chunk details for auth:\n");
    detail.debug_dump();
    if (!S3Option::get_instance()->is_auth_disabled()) {
      if (detail.get_size() == 0) {
        // Last chunk is size 0
        get_auth_client()->add_last_checksum_for_chunk(
            detail.get_signature(), detail.get_payload_hash());
      } else {
        get_auth_client()->add_checksum_for_chunk(detail.get_signature(),
                                                  detail.get_payload_hash());
      }
      auth_in_progress = true;  // this triggers auth
    }
  }
}

void S3PutMultiObjectAction::write_object(
    std::shared_ptr<S3AsyncBufferOptContainer> buffer) {

  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (request->is_chunked()) {
    // Also send any ready chunk data for auth
    send_chunk_details_if_any();
  }
  size_t content_length = buffer->get_content_length();

  if (content_length > motr_write_payload_size) {
    content_length = motr_write_payload_size;
  }
  motr_writer->write_content(
      std::bind(&S3PutMultiObjectAction::write_object_successful, this),
      std::bind(&S3PutMultiObjectAction::write_object_failed, this),
      buffer->get_buffers(content_length), buffer->size_of_each_evbuf);

  motr_write_in_progress = true;

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::write_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  motr_write_in_progress = false;

  request->get_buffered_input()->flush_used_buffers();

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  if (request->is_s3_client_read_error()) {
    client_read_error();
    return;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Write successful\n");
  if (request->is_chunked()) {
    if (auth_failed) {
      s3_put_action_state = S3PutPartActionState::writeFailed;
      set_s3_error("SignatureDoesNotMatch");
      send_response_to_s3_client();
      return;
    }
  }
  if (/* buffered data len is at least equal max we can write to motr in one
         write */
      request->get_buffered_input()->get_content_length() >=
          motr_write_payload_size ||
      /* we have all the data buffered and ready to write */
      (request->get_buffered_input()->is_freezed() &&
       request->get_buffered_input()->get_content_length() > 0)) {
    write_object(request->get_buffered_input());
  } else if (request->get_buffered_input()->is_freezed() &&
             request->get_buffered_input()->get_content_length() == 0) {
    motr_write_completed = true;
    s3_put_action_state = S3PutPartActionState::writeComplete;
    if (request->is_chunked()) {
      if (auth_completed) {
        next();
      } else {
        // else wait for auth to complete
        send_chunk_details_if_any();
      }
    } else {
      next();
    }
  } else if (!request->get_buffered_input()->is_freezed()) {
    // else we wait for more incoming data
    request->resume();
  }
}

void S3PutMultiObjectAction::write_object_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Write to motr failed\n");

  motr_write_in_progress = false;
  motr_write_completed = true;
  s3_put_action_state = S3PutPartActionState::writeFailed;

  request->get_buffered_input()->flush_used_buffers();

  if (request->is_s3_client_read_error()) {
    client_read_error();
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  if (request->is_chunked()) {
    write_failed = true;
    request->pause();  // pause any further reading from client.
    get_auth_client()->abort_chunk_auth_op();
    if (!auth_in_progress) {
      send_response_to_s3_client();
    }
  } else {
    send_response_to_s3_client();
  }
}

void S3PutMultiObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // to rest Date and Last-Modfied time object metadata
  part_metadata->reset_date_time_to_current();
  part_metadata->set_content_length(request->get_data_length_str());
  part_metadata->set_md5(motr_writer->get_content_md5());
  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      part_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);

  if (s3_fi_is_enabled("fail_save_part_mdata")) {
    s3_fi_enable_once("motr_kv_put_fail");
  }

  part_metadata->save(
      std::bind(&S3PutMultiObjectAction::save_object_metadata_success, this),
      std::bind(&S3PutMultiObjectAction::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutPartActionState::metadataSaved;
  next();
}

void S3PutMultiObjectAction::save_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutPartActionState::metadataSaveFailed;
  if (part_metadata->get_state() == S3PartMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Save of Part metadata failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Save of Part metadata failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi || old_object_oid.u_lo) {
    assert(!old_oid_str.empty());

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(
        old_oid_str, object_metadata->get_content_length());

    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding old_probable_del_rec with key [%s]\n",
           old_oid_rec_key.c_str());
    // TODO - Revisit later to see the changes needed here
    old_probable_del_rec.reset(new S3ProbableDeleteRecord(
        old_oid_rec_key, {0ULL, 0ULL}, std::to_string(part_number),
        old_object_oid, old_layout_id, part_metadata->get_part_index_oid(),
        bucket_metadata->get_objects_version_list_index_oid(), "",
        false /* force_delete */, true, part_metadata->get_part_index_oid()));

    probable_oid_list[old_oid_rec_key] = old_probable_del_rec->to_json();
  }
  // prepending a char depending on the size of the object (size based bucketing
  // of object)
  S3CommonUtilities::size_based_bucketing_of_objects(
      new_oid_str, request->get_content_length());

  s3_log(S3_LOG_DEBUG, request_id,
         "Adding new_probable_del_rec with key [%s]\n", new_oid_str.c_str());
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      new_oid_str, old_object_oid, std::to_string(part_number), new_object_oid,
      layout_id, part_metadata->get_part_index_oid(),
      bucket_metadata->get_objects_version_list_index_oid(), "",
      false /* force_delete */, true, part_metadata->get_part_index_oid()));
  // store new oid, key = newoid
  probable_oid_list[new_oid_str] = new_probable_del_rec->to_json();

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_oid, probable_oid_list,
      std::bind(&S3PutMultiObjectAction::next, this),
      std::bind(&S3PutMultiObjectAction::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutPartActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if ((auth_in_progress) &&
      (get_auth_client()->get_state() == S3AuthClientOpState::started)) {
    get_auth_client()->abort_chunk_auth_op();
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    // Send response with 'Service Unavailable' code.
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
  } else if (motr_writer != NULL) {
    // AWS adds explicit quotes "" to etag values.
    std::string e_tag = "\"" + motr_writer->get_content_md5() + "\"";

    request->set_out_header_value("ETag", e_tag);

    request->send_response(S3HttpSuccess200);
  } else {
    set_s3_error("InternalError");
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Connection", "close");
    request->send_response(error.get_http_status_code(), response_xml);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  request->resume(false);
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::startcleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // TODO: Perf - all/some of below tasks can be done in parallel
  // Any of the following steps fail, backgrounddelete will be able to perform
  // cleanups.
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;
  // Success conditions
  if (s3_put_action_state == S3PutPartActionState::completed) {
    s3_log(S3_LOG_DEBUG, request_id, "Cleanup old Object\n");
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // mark old OID for deletion in overwrite case, this optimizes
      // backgrounddelete decisions.
      ACTION_TASK_ADD(S3PutMultiObjectAction::mark_old_oid_for_deletion, this);
    }
    // remove new oid from probable delete list.
    ACTION_TASK_ADD(S3PutMultiObjectAction::remove_new_oid_probable_record,
                    this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // Object overwrite case, old object exists, delete it.
      ACTION_TASK_ADD(S3PutMultiObjectAction::delete_old_object, this);
      // If delete object is successful, attempt to delete old probable record
    }
  } else if (s3_put_action_state == S3PutPartActionState::newObjOidCreated ||
             s3_put_action_state == S3PutPartActionState::writeFailed ||
             s3_put_action_state == S3PutPartActionState::md5ValidationFailed ||
             s3_put_action_state == S3PutPartActionState::metadataSaveFailed) {
    // PUT is assumed to be failed with a need to rollback
    s3_log(S3_LOG_DEBUG, request_id,
           "Cleanup new Object: s3_put_action_state[%d]\n",
           s3_put_action_state);
    // Mark new OID for deletion, this optimizes backgrounddelete decisionss.
    ACTION_TASK_ADD(S3PutMultiObjectAction::mark_new_oid_for_deletion, this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // remove old oid from probable delete list.
      ACTION_TASK_ADD(S3PutMultiObjectAction::remove_old_oid_probable_record,
                      this);
    }
    ACTION_TASK_ADD(S3PutMultiObjectAction::delete_new_object, this);
    // If delete object is successful, attempt to delete new probable record
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "No Cleanup required: s3_put_action_state[%d]\n",
           s3_put_action_state);
    assert(s3_put_action_state == S3PutPartActionState::empty ||
           s3_put_action_state == S3PutPartActionState::validationFailed ||
           s3_put_action_state ==
               S3PutPartActionState::probableEntryRecordFailed ||
           s3_put_action_state ==
               S3PutPartActionState::newObjOidCreationFailed);
    // Nothing to undo
  }
  // Start running the cleanup task list
  start();
}
void S3PutMultiObjectAction::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  // update new oid, key = newoid, force_del = true
  new_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             new_oid_str, new_probable_del_rec->to_json(),
                             std::bind(&S3PutMultiObjectAction::next, this),
                             std::bind(&S3PutMultiObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  std::string prepended_new_oid_str = new_oid_str;
  // key = oldoid + "-" + newoid

  std::string old_oid_rec_key =
      old_oid_str + '-' + prepended_new_oid_str.erase(0, 1);

  // update old oid, force_del = true
  old_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             old_oid_rec_key, old_probable_del_rec->to_json(),
                             std::bind(&S3PutMultiObjectAction::next, this),
                             std::bind(&S3PutMultiObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                old_oid_rec_key,
                                std::bind(&S3PutMultiObjectAction::next, this),
                                std::bind(&S3PutMultiObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                new_oid_str,
                                std::bind(&S3PutMultiObjectAction::next, this),
                                std::bind(&S3PutMultiObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::delete_old_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT is success, we delete old object if present
  assert(old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL);

  // If old part exists and deletion of old is disabled, then return
  if ((old_object_oid.u_hi || old_object_oid.u_lo) &&
      S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    s3_log(S3_LOG_INFO, request_id,
           "Skipping deletion of old object. The old object will be deleted by "
           "BD.\n");
    // Call next task in the pipeline
    next();
    return;
  }
  motr_writer->set_oid(old_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutMultiObjectAction::remove_old_oid_probable_record, this),
      std::bind(&S3PutMultiObjectAction::next, this), old_object_oid,
      old_layout_id, object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectAction::delete_new_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT failed, then clean new object.
  assert(s3_put_action_state != S3PutPartActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);

  motr_writer->set_oid(new_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutMultiObjectAction::remove_new_oid_probable_record, this),
      std::bind(&S3PutMultiObjectAction::next, this), new_object_oid, layout_id,
      object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

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

#include "s3_put_multiobjectcopy_action.h"
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
#include "s3_object_data_copier.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3PutMultiObjectCopyAction::S3PutMultiObjectCopyAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<S3AuthClientFactory> auth_factory)
    : S3PutObjectActionBase(std::move(req), std::move(bucket_meta_factory),
                            std::move(object_meta_factory), std::move(motr_api),
                            std::move(motr_s3_writer_factory),
                            std::move(kv_writer_factory)),
      auth_failed(false),
      auth_in_progress(false),
      auth_completed(false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  part_number = get_part_number();
  upload_id = request->get_query_string_value("uploadId");

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Upload Part Copy. Bucket[%s] Object[%s]\
         Part[%d] for UploadId[%s]\n",
         request->get_bucket_name().c_str(), request->get_object_name().c_str(),
         part_number, upload_id.c_str());

  action_uses_cleanup = true;
  s3_put_action_state = S3PutObjectActionState::empty;
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

  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);

  setup_steps();
}

void S3PutMultiObjectCopyAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  ACTION_TASK_ADD(
      S3PutMultiObjectCopyAction::set_source_bucket_authorization_metadata,
      this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::check_source_bucket_authorization,
                  this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::check_part_details, this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::fetch_multipart_metadata, this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::fetch_part_info, this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::create_part_object, this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::copy_object, this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::save_metadata, this);
  ACTION_TASK_ADD(S3PutMultiObjectCopyAction::send_response_to_s3_client, this);
}

void S3PutMultiObjectCopyAction::set_source_bucket_authorization_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  auth_acl = request->get_default_acl();
  auth_client->set_get_method = true;

  request->reset_action_list();
  auth_client->set_entity_path("/" + additional_bucket_name + "/" +
                               additional_object_name);

  auth_client->set_acl_and_policy(
      additional_object_metadata->get_encoded_object_acl(),
      additional_bucket_metadata->get_policy_as_json());
  request->set_action_str("GetObject");
  request->reset_action_list();

  if (!additional_object_metadata->get_tags().empty()) {
    request->set_action_list("GetObjectTagging");
  }
  if (!request->get_header_value("x-amz-acl").empty()) {
    request->set_action_list("GetObjectAcl");
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutMultiObjectCopyAction::check_source_bucket_authorization() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  auth_client->check_authorization(
      std::bind(&S3PutMultiObjectCopyAction::
                     check_source_bucket_authorization_success,
                this),
      std::bind(
          &S3PutMultiObjectCopyAction::check_source_bucket_authorization_failed,
          this));
}

void S3PutMultiObjectCopyAction::check_source_bucket_authorization_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void
S3PutMultiObjectCopyAction::check_destination_bucket_authorization_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::validationFailed;
  std::string error_code = auth_client->get_error_code();

  set_s3_error(error_code);
  s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
         error_code.c_str());

  if (request->client_connected()) {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::check_source_bucket_authorization_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::validationFailed;
  std::string error_code = auth_client->get_error_code();

  set_s3_error(error_code);
  s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
         error_code.c_str());

  if (request->client_connected()) {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::check_part_details() {
  // "Part numbers can be any number from 1 to 10,000, inclusive."
  // https://docs.aws.amazon.com/en_us/AmazonS3/latest/API/API_UploadPart.html
  if (part_number < MINIMUM_PART_NUMBER || part_number > MAXIMUM_PART_NUMBER) {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidPart");
    send_response_to_s3_client();
  } else if (request->get_header_size() > MAX_HEADER_SIZE ||
             request->get_user_metadata_size() > MAX_USER_METADATA_SIZE) {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("MetadataTooLarge");
    send_response_to_s3_client();
  } else if ((request->get_object_name()).length() > MAX_OBJECT_KEY_LENGTH) {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("KeyTooLongError");
    send_response_to_s3_client();
  } else {
    next();
  }
}

void S3PutMultiObjectCopyAction::fetch_multipart_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string multipart_key_name =
      request->get_object_name() + EXTENDED_METADATA_OBJECT_SEP + upload_id;
  object_multipart_metadata =
      object_mp_metadata_factory->create_object_mp_metadata_obj(
          request, bucket_metadata->get_multipart_index_layout(), upload_id);

  // In multipart table key is object_name + |uploadid
  object_multipart_metadata->rename_object_name(multipart_key_name);

  object_multipart_metadata->load(
      std::bind(&S3PutMultiObjectCopyAction::next, this),
      std::bind(&S3PutMultiObjectCopyAction::fetch_multipart_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::fetch_multipart_failed() {
  // Log error
  s3_put_action_state = S3PutObjectActionState::validationFailed;
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

void S3PutMultiObjectCopyAction::fetch_part_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  part_metadata = part_metadata_factory->create_part_metadata_obj(
      request, object_multipart_metadata->get_part_index_layout(), upload_id,
      part_number);

  part_metadata->load(
      std::bind(&S3PutMultiObjectCopyAction::fetch_part_info_success, this),
      std::bind(&S3PutMultiObjectCopyAction::fetch_part_info_failed, this),
      part_number);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::fetch_part_info_success() {
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

void S3PutMultiObjectCopyAction::fetch_part_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata->get_state() == S3PartMetadataState::missing) {
    next();
  } else {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::_set_layout_id(int layout_id) {
  assert(layout_id > 0 && layout_id < 15);
  this->layout_id = layout_id;

  motr_write_payload_size =
      S3Option::get_instance()->get_motr_write_payload_size(layout_id);
}

void S3PutMultiObjectCopyAction::create_part_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  create_object();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::copy_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::save_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string s_md5_got = request->get_header_value("content-md5");
  if (!s_md5_got.empty() && !motr_writer->content_md5_matches(s_md5_got)) {
    s3_put_action_state = S3PutObjectActionState::metadataSaveFailed;
    s3_log(S3_LOG_ERROR, request_id, "Content MD5 mismatch\n");
    set_s3_error("BadDigest");
    send_response_to_s3_client();
    return;
  }
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
      std::bind(&S3PutMultiObjectCopyAction::save_object_metadata_success,
                this),
      std::bind(&S3PutMultiObjectCopyAction::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::metadataSaved;
  next();
}

void S3PutMultiObjectCopyAction::save_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::metadataSaveFailed;
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

void S3PutMultiObjectCopyAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi || old_object_oid.u_lo) {
    assert(!old_oid_str.empty());

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(
        old_oid_str, part_metadata->get_content_length());

    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding old_probable_del_rec with key [%s]\n",
           old_oid_rec_key.c_str());
    old_probable_del_rec.reset(new S3ProbableDeleteRecord(
        old_oid_rec_key, {0ULL, 0ULL},
        object_multipart_metadata->get_object_name(), old_object_oid,
        old_layout_id, object_multipart_metadata->get_pvid_str(),
        bucket_metadata->get_multipart_index_layout().oid,
        bucket_metadata->get_objects_version_list_index_layout().oid, "",
        false /* force_delete */, true,
        part_metadata->get_part_index_layout().oid, 0,
        strtoul(part_metadata->get_part_number().c_str(), NULL, 0),
        bucket_metadata->get_extended_metadata_index_layout().oid));

    probable_oid_list[old_oid_rec_key] = old_probable_del_rec->to_json();
  }
  // prepending a char depending on the size of the object (size based bucketing
  // of object)
  S3CommonUtilities::size_based_bucketing_of_objects(
      new_oid_str, request->get_content_length());

  s3_log(S3_LOG_DEBUG, request_id,
         "Adding new_probable_del_rec with key [%s]\n", new_oid_str.c_str());
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      new_oid_str, old_object_oid, object_multipart_metadata->get_object_name(),
      new_object_oid, layout_id, object_multipart_metadata->get_pvid_str(),
      bucket_metadata->get_multipart_index_layout().oid,
      bucket_metadata->get_objects_version_list_index_layout().oid, "",
      false /* force_delete */, true,
      part_metadata->get_part_index_layout().oid, 1,
      strtoul(part_metadata->get_part_number().c_str(), NULL, 0),
      bucket_metadata->get_extended_metadata_index_layout().oid));
  // store new oid, key = newoid
  probable_oid_list[new_oid_str] = new_probable_del_rec->to_json();

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, probable_oid_list,
      std::bind(&S3PutMultiObjectCopyAction::next, this),
      std::bind(&S3PutMultiObjectCopyAction::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void
S3PutMultiObjectCopyAction::add_object_oid_to_probable_dead_oid_list_failed() {
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

void S3PutMultiObjectCopyAction::send_response_to_s3_client() {
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
      if (reject_if_shutting_down()) {
        int retry_after_period =
            S3Option::get_instance()->get_s3_retry_after_sec();
        request->set_out_header_value("Retry-After",
                                      std::to_string(retry_after_period));
      } else {
        request->set_out_header_value("Retry-After", "1");
      }
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

void S3PutMultiObjectCopyAction::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  // update new oid, key = newoid, force_del = true
  new_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, new_oid_str,
      new_probable_del_rec->to_json(),
      std::bind(&S3PutMultiObjectCopyAction::next, this),
      std::bind(&S3PutMultiObjectCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::mark_old_oid_for_deletion() {
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
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, old_oid_rec_key,
      old_probable_del_rec->to_json(),
      std::bind(&S3PutMultiObjectCopyAction::next, this),
      std::bind(&S3PutMultiObjectCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      global_probable_dead_object_list_index_layout, old_oid_rec_key,
      std::bind(&S3PutMultiObjectCopyAction::next, this),
      std::bind(&S3PutMultiObjectCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      global_probable_dead_object_list_index_layout, new_oid_str,
      std::bind(&S3PutMultiObjectCopyAction::next, this),
      std::bind(&S3PutMultiObjectCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::delete_old_object() {
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
      std::bind(&S3PutMultiObjectCopyAction::remove_old_oid_probable_record,
                this),
      std::bind(&S3PutMultiObjectCopyAction::next, this), old_object_oid,
      old_layout_id, object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultiObjectCopyAction::delete_new_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT failed, then clean new object.
  assert(s3_put_action_state != S3PutObjectActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);

  motr_writer->set_oid(new_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutMultiObjectCopyAction::remove_new_oid_probable_record,
                this),
      std::bind(&S3PutMultiObjectCopyAction::next, this), new_object_oid,
      layout_id, object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

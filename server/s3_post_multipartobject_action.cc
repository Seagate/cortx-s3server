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

#include <sstream>
#include "s3_post_multipartobject_action.h"
#include "s3_motr_layout.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"
#include "s3_stats.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_post_multipartobject_action.h"
#include "s3_m0_uint128_helper.h"
#include <evhttp.h>

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3PostMultipartObjectAction::S3PostMultipartObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3PutTagsBodyFactory> put_tags_body_factory,
    std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory)) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Initiate Multipart Upload. Bucket[%s]\
         Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  action_uses_cleanup =
      false;  // since startcleanup is noop and we use rollback
  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  oid = {0ULL, 0ULL};
  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &oid);

  tried_count = 0;

  old_oid = {0ULL, 0ULL};
  old_layout_id = -1;

  // Since we cannot predict the object size during multipart init, we use the
  // best recommended layout for better Performance
  layout_id =
      S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();


  if (object_mp_meta_factory) {
    object_mp_metadata_factory = std::move(object_mp_meta_factory);
  } else {
    object_mp_metadata_factory =
        std::make_shared<S3ObjectMultipartMetadataFactory>();
  }

  if (part_meta_factory) {
    part_metadata_factory = std::move(part_meta_factory);
  } else {
    part_metadata_factory = std::make_shared<S3PartMetadataFactory>();
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

void S3PostMultipartObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  if (!request->get_header_value("x-amz-tagging").empty()) {

    ACTION_TASK_ADD(
        S3PostMultipartObjectAction::validate_x_amz_tagging_if_present, this);
  }
  ACTION_TASK_ADD(S3PostMultipartObjectAction::check_bucket_object_state, this);
  ACTION_TASK_ADD(S3PostMultipartObjectAction::create_part_meta_index, this);
  ACTION_TASK_ADD(S3PostMultipartObjectAction::save_upload_metadata, this);
  ACTION_TASK_ADD(S3PostMultipartObjectAction::send_response_to_s3_client,
                  this);
  // ...
}

void S3PostMultipartObjectAction::fetch_object_info_failed() { next(); }

void S3PostMultipartObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::validate_x_amz_tagging_if_present() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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

void S3PostMultipartObjectAction::parse_x_amz_tagging_header(
    std::string content) {
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
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::validate_tags() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::check_bucket_object_state() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  S3BucketMetadataState bucket_state = bucket_metadata->get_state();
  if (bucket_state == S3BucketMetadataState::present) {
    S3Uuid uuid;
    upload_id = uuid.get_string_uuid();
    multipart_index_layout = bucket_metadata->get_multipart_index_layout();
    object_multipart_metadata =
        object_mp_metadata_factory->create_object_mp_metadata_obj(
            request, multipart_index_layout, upload_id);
    if (zero(multipart_index_layout.oid)) {
      // multipart_index_layout is null only when bucket metadata is corrupted.
      // user has to delete and recreate the bucket again to make it work.
      s3_log(S3_LOG_ERROR, request_id, "Bucket(%s) metadata is corrupted.\n",
             request->get_bucket_name().c_str());
      s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
             S3_IEM_METADATA_CORRUPTED_JSON);
      set_s3_error("MetaDataCorruption");
      send_response_to_s3_client();
      return;
    }
    object_multipart_metadata->set_oid(oid);
    S3ObjectMetadataState object_state = object_metadata->get_state();
    if (object_state == S3ObjectMetadataState::present) {
      s3_log(S3_LOG_DEBUG, request_id, "S3ObjectMetadataState::present\n");
      old_oid = object_metadata->get_oid();
      old_layout_id = object_metadata->get_layout_id();
      s3_log(S3_LOG_DEBUG, request_id,
             "Object with oid "
             "%" SCNx64 " : %" SCNx64 " already exists, creating new oid\n",
             old_oid.u_hi, old_oid.u_lo);

      object_multipart_metadata->set_old_oid(old_oid);
      object_multipart_metadata->set_old_layout_id(old_layout_id);
      object_multipart_metadata->set_old_version_id(
          object_metadata->get_obj_version_id());

      next();
    } else if (object_state == S3ObjectMetadataState::missing) {
      s3_log(S3_LOG_DEBUG, request_id, "S3ObjectMetadataState::missing\n");
      next();
    } else {
      s3_log(S3_LOG_DEBUG, request_id, "Failed to look up metadata.\n");
      set_s3_error("InternalError");
      send_response_to_s3_client();
    }
  } else {
    if (bucket_state == S3BucketMetadataState::missing) {
      s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
      set_s3_error("NoSuchBucket");
    } else {
      set_s3_error("InternalError");
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::create_part_meta_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  part_metadata =
      part_metadata_factory->create_part_metadata_obj(request, upload_id, 0);
  part_metadata->create_index(
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index_successful,
                this),
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::create_part_meta_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  object_multipart_metadata->set_part_index_layout(
      part_metadata->get_part_index_layout());
  add_task_rollback(std::bind(
      &S3PostMultipartObjectAction::rollback_create_part_meta_index, this));
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::create_part_meta_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata->get_state() == S3PartMetadataState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id, "Multipart index creation failed\n");
    set_s3_error("ServiceUnavailable");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::rollback_create() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  motr_writer->delete_object(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(&S3PostMultipartObjectAction::rollback_create_failed, this),
      oid, layout_id, object_multipart_metadata->get_pvid());

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::rollback_create_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_writer->get_state() != S3MotrWiterOpState::missing) {
    s3_log(S3_LOG_ERROR, request_id,
           "Deletion of object failed, this oid will be stale in Motr: "
           "%" SCNx64 " : %" SCNx64 "\n",
           oid.u_hi, oid.u_lo);
    // s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
    //     S3_IEM_DELETE_OBJ_FAIL_JSON);
  }
  rollback_next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::rollback_create_part_meta_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  part_metadata->remove_index(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(
          &S3PostMultipartObjectAction::rollback_create_part_meta_index_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::rollback_create_part_meta_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  const auto& part_index_layout = part_metadata->get_part_index_layout();
  s3_log(S3_LOG_ERROR, request_id,
         "Deletion of index failed, this oid will be stale in Motr"
         "%" SCNx64 " : %" SCNx64 "\n",
         part_index_layout.oid.u_hi, part_index_layout.oid.u_lo);
  // s3_iem(LOG_ERR, S3_IEM_DELETE_IDX_FAIL, S3_IEM_DELETE_IDX_FAIL_STR,
  //     S3_IEM_DELETE_IDX_FAIL_JSON);
  rollback_next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::save_upload_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string multipart_object_name =
      request->get_object_name() + "|" + upload_id;
  object_multipart_metadata->rename_object_name(multipart_object_name);
  object_multipart_metadata->set_layout_id(layout_id);
  // Save empty pv id string
  object_multipart_metadata->set_pvid(nullptr);

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      object_multipart_metadata->add_user_defined_attribute(it.first,
                                                            it.second);
    }
  }

  object_multipart_metadata->set_tags(new_object_tags_map);
  // to rest Date and Last-Modfied time object metadata
  object_multipart_metadata->reset_date_time_to_current();
  object_multipart_metadata->set_oid(oid);
  object_multipart_metadata->set_content_type(request->get_content_type());
  object_multipart_metadata->set_part_index_layout(
      part_metadata->get_part_index_layout());
  object_multipart_metadata->regenerate_version_id();

  object_multipart_metadata->save(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::save_upload_metadata_failed,
                this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "post_multipartobject_action_save_upload_metadata_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::save_upload_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id, "save upload index metadata failed\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (S3Option::get_instance()->is_getoid_enabled()) {

    request->set_out_header_value("x-stx-oid",
                                  S3M0Uint128Helper::to_string(oid));
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

    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }
    request->send_response(error.get_http_status_code(), response_xml);
  } else if ((object_multipart_metadata &&
              object_multipart_metadata->get_state() ==
                  S3ObjectMetadataState::saved) &&
             (part_metadata &&
              (part_metadata->get_state() ==
                   S3PartMetadataState::store_created ||
               part_metadata->get_state() == S3PartMetadataState::present))) {
    request->set_out_header_value("UploadId", upload_id);

    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<InitiateMultipartUploadResult xmlns=\"http://s3\">"
        << "<Bucket>" << request->get_bucket_name() << "</Bucket>"
        << "<Key>" << request->get_object_name() << "</Key>"
        << "<UploadId>" << upload_id << "</UploadId>"
        << "</InitiateMultipartUploadResult>";

    std::string response = oss.str();
    oss.clear();

    request->send_response(S3HttpSuccess200, std::move(response));
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostMultipartObjectAction::startcleanup() {
  // Clean up utilizes rollback task list
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_multipart_metadata &&
      object_multipart_metadata->get_state() == S3ObjectMetadataState::saved) {
    assert(!is_error_state() && get_s3_error_code().empty());
    // If there are no errors nothing to rollback.
    done();
  } else {
    rollback_start();
  }
}

void S3PostMultipartObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

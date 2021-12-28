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

#include "s3_delete_multiple_objects_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_m0_uint128_helper.h"
#include "s3_common_utilities.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

// AWS allows to delete maximum 1000 objects in one call
#define MAX_OBJS_ALLOWED_TO_DELETE 1000

S3DeleteMultipleObjectsAction::S3DeleteMultipleObjectsAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_md_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_md_factory,
    std::shared_ptr<S3MotrWriterFactory> writer_factory,
    std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kvs_writer_factory)
    : S3BucketAction(std::move(req), std::move(bucket_md_factory), false),
      delete_index_in_req(0),
      at_least_one_delete_successful(false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Delete Multiple Objects API. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  if (object_md_factory) {
    object_metadata_factory = std::move(object_md_factory);
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }

  if (writer_factory) {
    motr_writer_factory = std::move(writer_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  if (kvs_reader_factory) {
    motr_kvs_reader_factory = std::move(kvs_reader_factory);
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }

  if (kvs_writer_factory) {
    motr_kvs_writer_factory = std::move(kvs_writer_factory);
  } else {
    motr_kvs_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  std::shared_ptr<MotrAPI> s3_motr_api = std::make_shared<ConcreteMotrAPI>();

  motr_kv_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);

  motr_kv_writer =
      motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);

  motr_writer = motr_writer_factory->create_motr_writer(request);

  setup_steps();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3DeleteMultipleObjectsAction::validate_request, this);
  ACTION_TASK_ADD(S3DeleteMultipleObjectsAction::fetch_objects_info, this);
  // ACTION_TASK_ADD(S3DeleteMultipleObjectsAction::save_data_usage, this);
  // ACTION_TASK_ADD(S3DeleteMultipleObjectsAction::fetch_objects_extended_info,
  //                this);
  // Delete will be cycling between delete_objects_metadata and delete_objects
  ACTION_TASK_ADD(S3DeleteMultipleObjectsAction::send_response_to_s3_client,
                  this);
  // ...
}

void S3DeleteMultipleObjectsAction::validate_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (request->has_all_body_content()) {
    validate_request_body(request->get_full_body_content_as_string());
  } else {
    request->listen_for_incoming_data(
        std::bind(&S3DeleteMultipleObjectsAction::consume_incoming_content,
                  this),
        request->get_data_length() /* we ask for all */
        );
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::consume_incoming_content() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (request->is_s3_client_read_error()) {
    client_read_error();
  } else if (request->has_all_body_content()) {
    validate_request_body(request->get_full_body_content_as_string());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::validate_request_body(std::string content) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  MD5hash calc_md5(NULL, true);
  calc_md5.Update(content.c_str(), content.length());
  calc_md5.Finalize();

  std::string req_md5_str = request->get_header_value("content-md5");
  std::string calc_md5_str = calc_md5.get_md5_base64enc_string();
  if (calc_md5_str != req_md5_str) {
    // Request payload was corrupted in transit.
    set_s3_error("BadDigest");
    send_response_to_s3_client();
  } else {
    delete_request.initialize(request, content);
    if (delete_request.isOK()) {
      // AWS allows to delete maximum 1000 objects in one call
      if (delete_request.get_count() > MAX_OBJS_ALLOWED_TO_DELETE) {
        set_s3_error("MaxMessageLengthExceeded");
        send_response_to_s3_client();
      } else {
        next();
      }
    } else {
      invalid_request = true;
      set_s3_error("MalformedXML");
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Fetching of bucket information failed\n");
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

void S3DeleteMultipleObjectsAction::fetch_objects_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  object_list_index_layout = bucket_metadata->get_object_list_index_layout();
  extended_list_index_layout =
      bucket_metadata->get_extended_metadata_index_layout();
  if (zero(object_list_index_layout.oid)) {
    for (const auto& key : keys_to_delete) {
      delete_objects_response.add_success(key);
    }
    next();
  } else {
    if (delete_index_in_req < delete_request.get_count()) {
      keys_to_delete.clear();
      keys_to_delete = delete_request.get_keys(
          delete_index_in_req,
          S3Option::get_instance()->get_motr_idx_fetch_count());
      if (s3_fi_is_enabled("fail_fetch_objects_info")) {
        s3_fi_enable_once("motr_kv_get_fail");
      }
      motr_kv_reader->get_keyval(
          object_list_index_layout, keys_to_delete,
          std::bind(
              &S3DeleteMultipleObjectsAction::fetch_objects_info_successful,
              this),
          std::bind(&S3DeleteMultipleObjectsAction::fetch_objects_info_failed,
                    this));
      delete_index_in_req += keys_to_delete.size();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::fetch_objects_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    for (auto& key : keys_to_delete) {
      delete_objects_response.add_success(key);
    }
    if (delete_index_in_req < delete_request.get_count()) {
      fetch_objects_info();
    } else {
      next();
    }
  } else {
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::fetch_objects_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Create a list of objects found to be deleted

  const auto& kvps = motr_kv_reader->get_key_values();
  objects_metadata.clear();
  extended_oids_to_delete.clear();
  objects_metadata_with_extends.clear();
  objects_metadata_with_extends_index = 0;
  extended_layout_id_for_objs_to_delete.clear();
  extended_pv_ids_to_delete.clear();

  bool atleast_one_json_error = false;
  bool all_had_json_error = true;

  for (const auto& kv : kvps) {
    if ((kv.second.first == 0) && (!kv.second.second.empty())) {
      s3_log(S3_LOG_DEBUG, request_id, "Found Object metadata for = %s\n",
             kv.first.c_str());

      auto object = object_metadata_factory->create_object_metadata_obj(
          request, bucket_metadata->get_object_list_index_layout(),
          bucket_metadata->get_objects_version_list_index_layout());

      if (object->from_json(kv.second.second) != 0 ||
          !delete_request.validate_attrs(object->get_bucket_name(),
                                         object->get_object_name())) {
        atleast_one_json_error = true;
        s3_log(S3_LOG_ERROR, request_id,
               "Invalid index value. Index oid = "
               "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
               object_list_index_layout.oid.u_hi,
               object_list_index_layout.oid.u_lo, kv.first.c_str(),
               kv.second.second.c_str());
        delete_objects_response.add_failure(kv.first, "InternalError");
        object->mark_invalid();
      } else {
        all_had_json_error = false;  // at least one good object to delete
        if (object->get_number_of_fragments() > 0) {
          objects_metadata_with_extends.push_back(object);
        }
        objects_metadata.push_back(object);
      }
    } else {
      s3_log(S3_LOG_DEBUG, request_id, "Object metadata missing for = %s\n",
             kv.first.c_str());
      delete_objects_response.add_success(kv.first);
    }
  }
  if (atleast_one_json_error) {
    // s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
    //     S3_IEM_METADATA_CORRUPTED_JSON);
    s3_log(S3_LOG_DEBUG, request_id, "metadata may be inappropriate\n");
  }
  if (all_had_json_error) {
    // Fetch more if present, else just respond
    if (delete_index_in_req < delete_request.get_count()) {
      // Try to delete the remaining
      fetch_objects_info();
    } else {
      if (!at_least_one_delete_successful) {
        set_s3_error("InternalError");
        send_response_to_s3_client();
      } else {
        next();
      }
    }
  } else {
    // next();
    fetch_objects_extended_info();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::fetch_objects_extended_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (objects_metadata_with_extends.size() == 0) {
    add_object_oid_to_probable_dead_oid_list();
  } else {
    object_metadata =
        objects_metadata_with_extends[objects_metadata_with_extends_index++];
    // Read the extended parts of the object from extended index table
    std::shared_ptr<S3ObjectExtendedMetadata> extended_obj_metadata =
        object_metadata_factory->create_object_ext_metadata_obj(
            request, object_metadata->get_bucket_name(),
            object_metadata->get_object_name(),
            object_metadata->get_obj_version_key(),
            object_metadata->get_number_of_parts(),
            object_metadata->get_number_of_fragments(),
            bucket_metadata->get_extended_metadata_index_layout());
    object_metadata->set_extended_object_metadata(extended_obj_metadata);
    extended_obj_metadata->load(
        std::bind(&S3DeleteMultipleObjectsAction::
                       fetch_objects_extended_info_successful,
                  this),
        std::bind(
            &S3DeleteMultipleObjectsAction::fetch_objects_extended_info_failed,
            this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::fetch_objects_extended_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (objects_metadata_with_extends_index <
      objects_metadata_with_extends.size()) {
    fetch_objects_extended_info();
  } else {
    add_object_oid_to_probable_dead_oid_list();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::fetch_objects_extended_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_WARN, request_id,
         "Failed to fetch object %s metadata, this may remain stale\n",
         object_metadata->get_object_name().c_str());
  object_metadata->set_extended_object_metadata(nullptr);
  if (objects_metadata_with_extends_index <
      objects_metadata_with_extends.size()) {
    fetch_objects_extended_info();
  } else {
    add_object_oid_to_probable_dead_oid_list();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::save_data_usage() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  int64_t inc_object_count = 0;
  int64_t inc_obj_size = 0;

  for (auto& object_metadata : objects_metadata) {
    if (object_metadata->get_state() != S3ObjectMetadataState::invalid) {
      inc_object_count -= 1;
      inc_obj_size -= (object_metadata->get_content_length());
    }
  }

  S3DataUsageCache::update_data_usage(
      request, bucket_metadata, inc_object_count, inc_obj_size,
      std::bind(&S3DeleteMultipleObjectsAction::delete_objects_metadata, this),
      std::bind(&S3DeleteMultipleObjectsAction::save_data_usage_failed, this));

  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit\n", __func__);
}

void S3DeleteMultipleObjectsAction::revert_data_usage() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  int64_t inc_object_count = 0;
  int64_t inc_obj_size = 0;

  for (auto& object_metadata : objects_metadata) {
    if (object_metadata->get_state() != S3ObjectMetadataState::invalid) {
      inc_object_count += 1;
      inc_obj_size += (object_metadata->get_content_length());
    }
  }

  S3DataUsageCache::update_data_usage(
      request, bucket_metadata, inc_object_count, inc_obj_size,
      std::bind(&S3DeleteMultipleObjectsAction::delete_objects_metadata_failed,
                this),
      std::bind(&S3DeleteMultipleObjectsAction::delete_objects_metadata_failed,
                this));

  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit\n", __func__);
}

// TODO : how to handle failures to save bucket counters at this stage.
// Currently just logging error and moving ahead.
void S3DeleteMultipleObjectsAction::save_data_usage_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "failed to save Data Usage");
  set_s3_error("InternalError");
  next();
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit\n", __func__);
}

void S3DeleteMultipleObjectsAction::_add_object_oid_to_probable_dead_oid_list(
    const std::shared_ptr<S3ObjectMetadata>& obj,
    std::map<std::string, std::string>& delete_list) {
  if (obj->get_state() != S3ObjectMetadataState::invalid) {
    std::string oid_str = S3M0Uint128Helper::to_string(obj->get_oid());
    assert(!oid_str.empty());
    // Add error when any key is empty
    if (oid_str.empty()) {
      s3_log(S3_LOG_ERROR, request_id,
             "Invalid object metadata with empty object OID\n");
    }

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(
        oid_str, obj->get_content_length());

    s3_log(S3_LOG_DEBUG, request_id, "Adding probable_del_rec with key [%s]\n",
           oid_str.c_str());

    probable_oid_list[oid_str] =
        std::unique_ptr<S3ProbableDeleteRecord>(new S3ProbableDeleteRecord(
            oid_str, {0ULL, 0ULL}, obj->get_object_name(), obj->get_oid(),
            obj->get_layout_id(), obj->get_pvid_str(),
            bucket_metadata->get_object_list_index_layout().oid,
            bucket_metadata->get_objects_version_list_index_layout().oid,
            obj->get_version_key_in_index(), false /* force_delete */));
    delete_list[oid_str] = probable_oid_list[oid_str]->to_json();
  }
}

void S3DeleteMultipleObjectsAction::_add_oids_to_probable_dead_oid_list(
    const std::shared_ptr<S3ObjectMetadata>& obj_metadata,
    std::map<std::string, std::string>& delete_list) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::vector<std::string> extended_keys_list;
  unsigned int total_objects = 0;
  if (obj_metadata->get_number_of_parts() == 0) {
    // Object is only fragmented then:= total objects = fragments + 1
    // Base object itself a fragment
    // TODO for Faulttolerance verify is the value correct
    total_objects = obj_metadata->get_number_of_fragments() + 1;
  } else {
    // There is no fragment in base metadata of multipart uploaded object
    // All parts constitute fragment
    total_objects = obj_metadata->get_number_of_fragments();
    s3_log(S3_LOG_DEBUG, request_id,
           "Multipart uploaded object with %d parts\n", total_objects);
  }
  s3_log(S3_LOG_DEBUG, request_id, "Total extends : (%u) for %s\n",
         total_objects, obj_metadata->get_object_name().c_str());
  const std::shared_ptr<S3ObjectExtendedMetadata>& ext_obj =
      obj_metadata->get_extended_object_metadata();
  if (ext_obj == NULL) {
    return;
  }
  extended_keys_list = ext_obj->get_extended_entries_key_list();
  // insert entried to the vector which will be used in kv deletion
  extended_keys_list_to_be_deleted.insert(
      extended_keys_list_to_be_deleted.end(), extended_keys_list.begin(),
      extended_keys_list.end());
  const std::map<int, std::vector<struct s3_part_frag_context>>& ext_entries =
      ext_obj->get_raw_extended_entries();
  unsigned int ext_entry_index;
  for (unsigned int i = 0; i < total_objects; i++) {
    // multipart (may be fragmented) object
    // For now, ext_entry_index is always 0,
    // assuming multipart is not fragmented.
    ext_entry_index = 0;
    // For multipart, ext_entries are indexed from key=1 onwards
    const struct s3_part_frag_context& frag_info =
        (ext_entries.at(i + 1)).at(ext_entry_index);
    std::string oid_str = S3M0Uint128Helper::to_string(frag_info.motr_OID);
    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(oid_str,
                                                       frag_info.item_size);
    std::unique_ptr<S3ProbableDeleteRecord> probable_del_rec;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding part/fragment probable del rec with key [%s]\n",
           oid_str.c_str());
    std::string pvid_str;
    S3M0Uint128Helper::to_string(frag_info.PVID, pvid_str);
    probable_oid_list[oid_str] =
        std::unique_ptr<S3ProbableDeleteRecord>(new S3ProbableDeleteRecord(
            oid_str, {0ULL, 0ULL}, obj_metadata->get_object_name(),
            frag_info.motr_OID, frag_info.layout_id, pvid_str,
            bucket_metadata->get_objects_version_list_index_layout().oid,
            bucket_metadata->get_objects_version_list_index_layout().oid, "",
            false /* force_delete */, false,
            bucket_metadata->get_extended_metadata_index_layout().oid));
    delete_list[oid_str] = probable_oid_list[oid_str]->to_json();
    extended_oids_to_delete.push_back(
        probable_oid_list[oid_str]->get_current_object_oid());
    extended_layout_id_for_objs_to_delete.push_back(frag_info.layout_id);
    extended_pv_ids_to_delete.push_back(frag_info.PVID);
  }
}

void S3DeleteMultipleObjectsAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::map<std::string, std::string> delete_list;
  for (const auto& obj : objects_metadata) {
    if (obj->get_number_of_fragments() == 0) {
      _add_object_oid_to_probable_dead_oid_list(obj, delete_list);
    } else {
      _add_oids_to_probable_dead_oid_list(obj, delete_list);
    }
  }
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, delete_list,
      std::bind(&S3DeleteMultipleObjectsAction::save_data_usage, this),
      std::bind(&S3DeleteMultipleObjectsAction::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::
    add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::delete_objects_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::vector<std::string> keys;
  for (auto& obj : objects_metadata) {
    if (obj->get_state() != S3ObjectMetadataState::invalid) {
      keys.push_back(obj->get_object_name());
    }
  }
  if (s3_fi_is_enabled("fail_delete_objects_metadata")) {
    s3_fi_enable_once("motr_kv_delete_fail");
  }
  motr_kv_writer->delete_keyval(
      object_list_index_layout, keys,
      std::bind(&S3DeleteMultipleObjectsAction::delete_extended_metadata, this),
      std::bind(&S3DeleteMultipleObjectsAction::revert_data_usage, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::delete_extended_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (extended_keys_list_to_be_deleted.size() > 0) {
    motr_kv_writer->delete_keyval(
        extended_list_index_layout, extended_keys_list_to_be_deleted,
        std::bind(
            &S3DeleteMultipleObjectsAction::delete_extended_metadata_successful,
            this),
        std::bind(
            &S3DeleteMultipleObjectsAction::delete_extended_metadata_failed,
            this));
  } else {
    delete_extended_metadata_successful();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::delete_extended_metadata_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  at_least_one_delete_successful = true;
  for (auto& obj : objects_metadata) {
    delete_objects_response.add_success(obj->get_object_name());
    oids_to_delete.push_back(obj->get_oid());
    layout_id_for_objs_to_delete.push_back(obj->get_layout_id());
    pv_ids_to_delete.push_back(obj->get_pvid());
    if (extended_oids_to_delete.size() != 0) {
      oids_to_delete.insert(oids_to_delete.end(),
                            extended_oids_to_delete.begin(),
                            extended_oids_to_delete.end());
      layout_id_for_objs_to_delete.insert(
          layout_id_for_objs_to_delete.end(),
          extended_layout_id_for_objs_to_delete.begin(),
          extended_layout_id_for_objs_to_delete.end());
      pv_ids_to_delete.insert(pv_ids_to_delete.end(),
                              extended_pv_ids_to_delete.begin(),
                              extended_pv_ids_to_delete.end());
    }
  }

  if (delete_index_in_req < delete_request.get_count()) {
    // Try to fetch the remaining
    fetch_objects_info();
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::delete_extended_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Do logging of stale entries and then go ahead with OID deletion
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id,
           "Extended Object metadata delete operation failed due to pre launch "
           "failure this will lead to stale entries\n");
    for (auto& extended_obj : extended_keys_list_to_be_deleted) {
      s3_log(S3_LOG_WARN, request_id, "Extended entry %s will be stale",
             extended_obj.c_str());
    }

  } else {
    uint obj_index = 0;
    for (auto& extended_obj_keyname : extended_keys_list_to_be_deleted) {
      if ((motr_kv_writer->get_op_ret_code_for_del_kv(obj_index) != -ENOENT) &&
          (motr_kv_writer->get_op_ret_code_for_del_kv(obj_index) != 0)) {
        s3_log(S3_LOG_WARN, request_id, "Extended entry %s will be stale",
               extended_obj_keyname.c_str());
      }
      ++obj_index;
    }
  }
  delete_extended_metadata_successful();
}

void S3DeleteMultipleObjectsAction::delete_objects_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    s3_log(
        S3_LOG_DEBUG, request_id,
        "Object metadata delete operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    uint obj_index = 0;
    for (auto& obj : objects_metadata) {
      if (motr_kv_writer->get_op_ret_code_for_del_kv(obj_index) == -ENOENT) {
        at_least_one_delete_successful = true;
        delete_objects_response.add_success(obj->get_object_name());
      } else {
        delete_objects_response.add_failure(obj->get_object_name(),
                                            "InternalError");
      }
      ++obj_index;
    }
    if (delete_index_in_req < delete_request.get_count()) {
      // Try to fetch the remaining
      fetch_objects_info();
    } else {
      if (!at_least_one_delete_successful) {
        set_s3_error("InternalError");
        send_response_to_s3_client();
      } else {
        next();
      }
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (is_error_state() && !get_s3_error_code().empty()) {
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
  } else {
    std::string& response_xml =
        delete_objects_response.to_xml(delete_request.is_quiet());

    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, request_id, "Object list response_xml = %s\n",
           response_xml.c_str());

    request->send_response(S3HttpSuccess200, response_xml);
  }
  request->resume(false);

  cleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::cleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (oids_to_delete.empty()) {
    cleanup_oid_from_probable_dead_oid_list();
  } else {
    if (S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
      // All the object oids entries in probable list index, it will
      // be deleted later
      done();
    } else {
      // Now trigger the delete.
      motr_writer->delete_objects(
          oids_to_delete, layout_id_for_objs_to_delete, pv_ids_to_delete,
          std::bind(&S3DeleteMultipleObjectsAction::cleanup_successful, this),
          std::bind(&S3DeleteMultipleObjectsAction::cleanup_failed, this));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::cleanup_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::cleanup_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "delete_objects_failed called due to motr_entity_open failure\n");
    // failed to to perform delete object operation, so clear probable_oid_list
    // map to retain all the object oids entries in probable list index id
    probable_oid_list.clear();
  } else {
    uint obj_index = 0;
    for (auto& oid : oids_to_delete) {
      if (motr_writer->get_op_ret_code_for_delete_op(obj_index) != -ENOENT) {
        // object oid failed to delete, so erase the object oid from
        // probable_oid_list map and so that this object oid will be retained in
        // global_probable_dead_object_list_index_layout
        probable_oid_list.erase(S3M0Uint128Helper::to_string(oid));
      }
      ++obj_index;
    }
  }
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteMultipleObjectsAction::cleanup_oid_from_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::vector<std::string> clean_valid_oid_from_probable_dead_object_list_index;

  // final probable_oid_list map will have valid object oids
  // that needs to be cleanup from global_probable_dead_object_list_index_layout
  for (auto& kv : probable_oid_list) {
    clean_valid_oid_from_probable_dead_object_list_index.push_back(kv.first);
  }

  if (!clean_valid_oid_from_probable_dead_object_list_index.empty()) {
    motr_kv_writer->delete_keyval(
        global_probable_dead_object_list_index_layout,
        clean_valid_oid_from_probable_dead_object_list_index,
        std::bind(&Action::done, this), std::bind(&Action::done, this));
  } else {
    done();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

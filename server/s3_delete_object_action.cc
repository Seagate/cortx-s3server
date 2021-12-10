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

#include "s3_delete_object_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_m0_uint128_helper.h"
#include "s3_common_utilities.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3DeleteObjectAction::S3DeleteObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> writer_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory), false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Delete Object API. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  action_uses_cleanup = true;
  s3_del_obj_action_state = S3DeleteObjectActionState::empty;

  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }

  if (writer_factory) {
    motr_writer_factory = writer_factory;
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  if (kv_writer_factory) {
    mote_kv_writer_factory = kv_writer_factory;
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  setup_steps();
}

void S3DeleteObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  // We delete metadata first followed by object, since if we delete
  // the object first and say for some reason metadata clean up fails,
  // If this "oid" gets allocated to some other object, current object
  // metadata will point to someone else's object leading 2 problems:
  // accessing someone else's object and second retrying delete, deleting
  // someone else's object. Whereas deleting metadata first will worst case
  // lead to object leak in motr which can handle separately.
  // To delete stale objects: ref: MINT-602

  /*
  delete handler
    probable delete   // if bucket is suspended
    create delete marker //if bucket is enabled
  object_metadata_handler
    save delete_marker  // if bucket is suspended
    delete_metadata //if bucket is enabled
  send_responce_to_client
  */
  ACTION_TASK_ADD(S3DeleteObjectAction::delete_handler, this);
  ACTION_TASK_ADD(S3DeleteObjectAction::metadata_handler, this);
  ACTION_TASK_ADD(S3DeleteObjectAction::send_response_to_s3_client, this);
}

void S3DeleteObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_del_obj_action_state = S3DeleteObjectActionState::validationFailed;
  S3BucketMetadataState bucket_metadata_state = bucket_metadata->get_state();
  if (bucket_metadata_state == S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else if (bucket_metadata_state == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::fetch_ext_object_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  set_s3_error("InternalError");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::fetch_object_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_del_obj_action_state = S3DeleteObjectActionState::validationFailed;
  if (zero(obj_list_idx_lo.oid) ||
      (object_metadata->get_state() == S3ObjectMetadataState::missing)) {
    s3_log(S3_LOG_WARN, request_id, "Object not found\n");
  } else if (object_metadata->get_state() ==
             S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Object metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_WARN, request_id, "Failed to look up Object metadata\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::delete_handler() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  std::string bucket_versioning =
      bucket_metadata->get_bucket_versioning_status();

  if (bucket_versioning == "Enabled" &&
      !request->has_query_param_key("versionId")) {
    create_delete_marker();
    next();
  } else {
    populate_probable_dead_oid_list();
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::metadata_handler() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  std::string bucket_versioning =
      bucket_metadata->get_bucket_versioning_status();
  if (bucket_versioning == "Enabled" &&
      !request->has_query_param_key("versionId")) {
    save_delete_marker();
  } else {
    delete_metadata();
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::create_delete_marker() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  // create metadata
  delete_marker_metadata =
      object_metadata_factory->create_delete_marker_metadata_obj(
          request, bucket_metadata->get_object_list_index_layout(),
          bucket_metadata->get_objects_version_list_index_layout());

  // Generate a version id for the new object.
  delete_marker_metadata->regenerate_version_id();
  delete_marker_metadata->reset_date_time_to_current();

  // TODO: update null version

  s3_log(S3_LOG_DEBUG, request_id, "Add delete marker4\n");
  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Writing user metadata on object: [%s] -> [%s]\n",
             it.first.c_str(), it.second.c_str());
      delete_marker_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::save_delete_marker() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  delete_marker_metadata->save(
      std::bind(&S3DeleteObjectAction::save_delete_marker_success, this),
      std::bind(&S3DeleteObjectAction::save_delete_marker_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::save_delete_marker_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_del_obj_action_state = S3DeleteObjectActionState::deleteMarkerAdded;
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::save_delete_marker_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_del_obj_action_state = S3DeleteObjectActionState::addDeleteMarkerFailed;
  set_s3_error("InternalError");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::populate_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_metadata->get_number_of_fragments() == 0) {
    add_object_oid_to_probable_dead_oid_list();
  } else {
    add_oids_to_probable_dead_oid_list();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  oid_str = S3M0Uint128Helper::to_string(object_metadata->get_oid());
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  // prepending a char depending on the size of the object (size based bucketing
  // of object)
  S3CommonUtilities::size_based_bucketing_of_objects(
      oid_str, object_metadata->get_content_length());

  s3_log(S3_LOG_DEBUG, request_id, "Adding probable_del_rec with key [%s]\n",
         oid_str.c_str());
  probable_delete_rec.reset(new S3ProbableDeleteRecord(
      oid_str, {0ULL, 0ULL}, object_metadata->get_object_name(),
      object_metadata->get_oid(), object_metadata->get_layout_id(),
      object_metadata->get_pvid_str(),
      bucket_metadata->get_object_list_index_layout().oid,
      bucket_metadata->get_objects_version_list_index_layout().oid,
      object_metadata->get_version_key_in_index(), false /* force_delete */));

  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, oid_str,
      probable_delete_rec->to_json(),
      std::bind(&S3DeleteObjectAction::next, this),
      std::bind(&S3DeleteObjectAction::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_del_obj_action_state =
      S3DeleteObjectActionState::probableEntryRecordFailed;
  set_s3_error("InternalError");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::add_oids_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  size_t total_objects;
  bool is_multipart = false;
  if (object_metadata->get_number_of_parts() == 0) {
    // Object is only fragmented then:= total objects = fragments + 1
    // Base object itself a fragment
    total_objects = object_metadata->get_number_of_fragments() + 1;
  } else {
    // There is no fragment in base metadata of multipart uploaded object
    // All parts constitute fragment
    total_objects = object_metadata->get_number_of_fragments();
    s3_log(S3_LOG_DEBUG, stripped_request_id,
           "Multipart uploaded object with %zu parts\n", total_objects);
    is_multipart = true;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Total objects : (%zu)\n", total_objects);
  const std::shared_ptr<S3ObjectExtendedMetadata>& ext_obj =
      object_metadata->get_extended_object_metadata();
  const std::map<int, std::vector<struct s3_part_frag_context>>& ext_entries =
      ext_obj->get_raw_extended_entries();
  unsigned int ext_entry_index;
  for (unsigned int i = 0; i < total_objects; i++) {
    struct S3ExtendedObjectInfo obj_info;
    if (is_multipart) {
      // multipart (may be fragmented) object
      // For now, ext_entry_index is always 0,
      // assuming multipart is not fragmented.
      ext_entry_index = 0;
      // For multipart, ext_entries are indexed from key=1 onwards
      const struct s3_part_frag_context& frag_info =
          (ext_entries.at(i + 1)).at(ext_entry_index);
      obj_info.object_OID = frag_info.motr_OID;
      obj_info.object_layout = frag_info.layout_id;
      obj_info.object_pvid = frag_info.PVID;
      obj_info.object_size = frag_info.item_size;
      // Not populating obj_info.requested_object_size

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
      // Pass: part no, extended md index oid, version id, parent oid(dummy oid)
      probable_del_rec.reset(new S3ProbableDeleteRecord(
          oid_str, {0ULL, 0ULL}, object_metadata->get_object_name(),
          frag_info.motr_OID, frag_info.layout_id, pvid_str,
          bucket_metadata->get_object_list_index_layout().oid,
          bucket_metadata->get_objects_version_list_index_layout().oid,
          object_metadata->get_version_key_in_index(), false /* force_delete */,
          false, {0ULL, 0ULL}, 1, i + 1,
          bucket_metadata->get_extended_metadata_index_layout().oid,
          frag_info.versionID,
          object_metadata->get_oid() /* parent oid of multipart */));
      probable_del_rec_list.push_back(std::move(probable_del_rec));
      extended_objects.push_back(obj_info);
    } else {
      // For Fragments non multipart
      // TODO
    }
  }
  // Add one more entry for parent multipart object to erase it
  // from version index once it is marked for deletion
  if (is_multipart) {
    std::string oid_str =
        S3M0Uint128Helper::to_string(object_metadata->get_oid());
    // For multiart parent object, size is 0 (as it is dummy object)
    S3CommonUtilities::size_based_bucketing_of_objects(oid_str, 0);
    std::unique_ptr<S3ProbableDeleteRecord> probable_del_rec;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding multipart parent probable del rec with key [%s]\n",
           oid_str.c_str());
    probable_del_rec.reset(new S3ProbableDeleteRecord(
        oid_str, {0ULL, 0ULL}, object_metadata->get_object_name(),
        object_metadata->get_oid(), 0, "",
        bucket_metadata->get_object_list_index_layout().oid,
        bucket_metadata->get_objects_version_list_index_layout().oid,
        object_metadata->get_version_key_in_index(), false /* force_delete */,
        false, {0ULL, 0ULL}, 1,
        total_objects /* Total parts in parent object */,
        bucket_metadata->get_extended_metadata_index_layout().oid,
        object_metadata->get_obj_version_key(), {0ULL, 0ULL}));
    probable_del_rec_list.push_back(std::move(probable_del_rec));
  }
  next();
}

void S3DeleteObjectAction::delete_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  object_metadata->remove(
      std::bind(&S3DeleteObjectAction::delete_metadata_successful, this),
      std::bind(&S3DeleteObjectAction::delete_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::delete_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Deleted Object metadata\n");
  s3_del_obj_action_state = S3DeleteObjectActionState::metadataDeleted;
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::delete_metadata_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed to delete Object metadata\n");
  s3_del_obj_action_state = S3DeleteObjectActionState::metadataDeleteFailed;
  set_s3_error("InternalError");
  send_response_to_s3_client();
}

void S3DeleteObjectAction::send_response_to_s3_client() {
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
  } else if (delete_marker_metadata &&
             s3_del_obj_action_state ==
                 S3DeleteObjectActionState::deleteMarkerAdded) {
    // delete marker metadata responce
    request->set_out_header_value("x-amz-version-id",
                                  delete_marker_metadata->get_obj_version_id());
    request->set_out_header_value("x-amz-delete-marker", "true");
    request->send_response(S3HttpSuccess204);
  } else {
    assert(zero(obj_list_idx_lo.oid) ||
           object_metadata->get_state() == S3ObjectMetadataState::missing ||
           object_metadata->get_state() == S3ObjectMetadataState::deleted);
    request->send_response(S3HttpSuccess204);
  }
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::startcleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;
  if (s3_del_obj_action_state == S3DeleteObjectActionState::validationFailed ||
      s3_del_obj_action_state ==
          S3DeleteObjectActionState::probableEntryRecordFailed) {
    // Nothing to clean up
    done();
  } else {
    if (s3_del_obj_action_state == S3DeleteObjectActionState::metadataDeleted) {
      // Metadata deleted, so object is gone for S3 client, clean storage.
      if (object_metadata->get_number_of_fragments() > 0) {
        ACTION_TASK_ADD(S3DeleteObjectAction::mark_oids_for_deletion, this);
        ACTION_TASK_ADD(S3DeleteObjectAction::delete_objects, this);
      } else {
        ACTION_TASK_ADD(S3DeleteObjectAction::mark_oid_for_deletion, this);
        ACTION_TASK_ADD(S3DeleteObjectAction::delete_object, this);
      }
      // If delete object is successful, attempt to delete probable record
      // Start running the cleanup task list
      start();
    } else if (s3_del_obj_action_state ==
               S3DeleteObjectActionState::metadataDeleteFailed) {
      // Failed to delete metadata, so object is still live, remove probable rec
      ACTION_TASK_ADD(S3DeleteObjectAction::remove_probable_record, this);
      // Start running the cleanup task list
      start();
    } else if (s3_del_obj_action_state ==
               S3DeleteObjectActionState::addDeleteMarkerFailed) {
      // TODO: Perform cleanup, need change in background delete
      done();
    } else if (s3_del_obj_action_state ==
               S3DeleteObjectActionState::deleteMarkerAdded) {
      // Nothing to clean up
      done();
    } else {
      s3_log(S3_LOG_INFO, stripped_request_id, "Possible bug\n");
      assert(false);
      done();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::mark_oids_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (probable_del_rec_list.size() == 0) {
    next();
    return;
  }

  unsigned int kv_to_be_processed =
      probable_del_rec_list.size() - total_processed_count;
  unsigned int how_many_kv_to_write =
      ((kv_to_be_processed - MAX_OIDS_FOR_DELETION) > MAX_OIDS_FOR_DELETION)
          ? MAX_OIDS_FOR_DELETION
          : kv_to_be_processed;

  std::map<std::string, std::string> oids_key_value_in_json;
  for (auto& delete_rec : probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    std::string oid_rec_key = delete_rec->get_key();
    oids_key_value_in_json[oid_rec_key] = delete_rec->to_json();
  }
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  if (probable_del_rec_list.size() <= MAX_OIDS_FOR_DELETION) {
    motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                               oids_key_value_in_json,
                               std::bind(&S3DeleteObjectAction::next, this),
                               std::bind(&S3DeleteObjectAction::next, this));
  } else {
    motr_kv_writer->put_partial_keyval(
        global_probable_dead_object_list_index_layout, oids_key_value_in_json,
        std::bind(
            &S3DeleteObjectAction::save_partial_extended_metadata_successful,
            this, std::placeholders::_1),
        std::bind(&S3DeleteObjectAction::save_partial_extended_metadata_failed,
                  this, std::placeholders::_1),
        total_processed_count, how_many_kv_to_write);
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::save_partial_extended_metadata_successful(
    unsigned int processed_count) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  total_processed_count += processed_count;
  s3_log(S3_LOG_INFO, request_id,
         "Processed Count %u "
         "total_processed_count[%u]\n",
         processed_count, total_processed_count);
  if (total_processed_count < probable_del_rec_list.size()) {
    mark_oids_for_deletion();
  } else {
    // save_metadata();
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::save_partial_extended_metadata_failed(
    unsigned int processed_count) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, request_id,
         "Processed Count %u "
         "total_processed_count[%u]\n",
         processed_count, total_processed_count);
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::mark_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!oid_str.empty());

  probable_delete_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             oid_str, probable_delete_rec->to_json(),
                             std::bind(&S3DeleteObjectAction::next, this),
                             std::bind(&S3DeleteObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::delete_objects() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (!motr_writer) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }

  // If delayed delete set then let s3 background delete handle it
  if (S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Skipping deletion of objects, objects will be deleted by "
           "BD.\n");
    // Remove version metadata
    // Followed by fragment removal (key-val)
    object_metadata->remove_version_metadata(
        std::bind(&S3DeleteObjectAction::remove_fragments_keyval, this),
        std::bind(&S3DeleteObjectAction::remove_fragments_keyval, this));

    return;
  }
  size_t object_count = extended_objects.size();
  if (object_count > 0) {
    struct S3ExtendedObjectInfo obj_info = extended_objects.back();
    // motr_writer->set_oid();
    motr_writer->delete_object(
        std::bind(&S3DeleteObjectAction::delete_objects_success, this),
        std::bind(&S3DeleteObjectAction::next, this), obj_info.object_OID,
        obj_info.object_layout, obj_info.object_pvid);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Delete entries corresponding to old objects
// from extended md index
void S3DeleteObjectAction::remove_fragments_keyval() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Remove extended key-value only from table
  // Background delete will take care of deleting OIDs
  if (object_metadata && object_metadata->is_object_extended()) {
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_metadata =
        object_metadata->get_extended_object_metadata();
    if (ext_metadata->has_entries()) {
      // Delete fragments key value, if any
      // Background will take care of deleting OIDs
      ext_metadata->remove(std::bind(&S3DeleteObjectAction::next, this),
                           std::bind(&S3DeleteObjectAction::next, this));
    } else {
      // Extended object exist, but no fragments, move ahead
      next();
    }
  } else {
    // If this is not extended object, move ahead
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::delete_objects_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  size_t object_count = extended_objects.size();
  if (object_count == 0) {
    next();
    return;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Deleted old part object oid "
         "[%" SCNx64 " : %" SCNx64 "]",
         ((extended_objects.back()).object_OID).u_hi,
         ((extended_objects.back()).object_OID).u_lo);
  extended_objects.pop_back();

  if (extended_objects.size() > 0) {
    delete_objects();
  } else {
    // Remove version metadata
    object_metadata->remove_version_metadata(
        std::bind(&S3DeleteObjectAction::remove_fragments, this),
        std::bind(&S3DeleteObjectAction::remove_fragments, this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Delete entries corresponding to the object
// from extended index
void S3DeleteObjectAction::remove_fragments() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_metadata && object_metadata->is_object_extended()) {
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_metadata =
        object_metadata->get_extended_object_metadata();
    if (ext_metadata->has_entries()) {
      // Delete fragments, if any
      ext_metadata->remove(
          std::bind(&S3DeleteObjectAction::remove_fragments_successful, this),
          std::bind(&S3DeleteObjectAction::remove_fragments_successful, this));
    } else {
      // Extended object exist, but no fragments, delete probale index entries
      remove_oids_from_probable_record_index();
    }
  } else {
    // If this is not extended object, delete entries from probable index
    remove_oids_from_probable_record_index();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::remove_fragments_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id,
         "Removed extended metadata for Object [%s].\n",
         (object_metadata->get_object_name()).c_str());
  remove_oids_from_probable_record_index();
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
}

void S3DeleteObjectAction::remove_oids_from_probable_record_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (probable_del_rec_list.size() == 0) {
    next();
    return;
  }
  std::vector<std::string> keys_to_delete;
  for (auto& probable_rec : probable_del_rec_list) {
    std::string old_oid_rec_key = probable_rec->get_key();
    keys_to_delete.push_back(old_oid_rec_key);
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_layout,
                                keys_to_delete,
                                std::bind(&S3DeleteObjectAction::next, this),
                                std::bind(&S3DeleteObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::delete_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If old object exists and deletion of old is disabled, then return
  const m0_uint128& obj_oid = object_metadata->get_oid();
  if ((obj_oid.u_hi || obj_oid.u_lo) &&
      S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    s3_log(S3_LOG_INFO, request_id,
           "Skipping deletion of object with oid %" SCNx64 " : %" SCNx64
           ". The object will be deleted by BD.\n",
           obj_oid.u_hi, obj_oid.u_lo);
    // Call next task in the pipeline
    next();
    return;
  }
  // process to delete object
  if (!motr_writer) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }
  motr_writer->delete_object(
      std::bind(&S3DeleteObjectAction::remove_probable_record, this),
      std::bind(&S3DeleteObjectAction::next, this), obj_oid,
      object_metadata->get_layout_id(), object_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3DeleteObjectAction::remove_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_layout,
                                oid_str,
                                std::bind(&S3DeleteObjectAction::next, this),
                                std::bind(&S3DeleteObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// To delete a object, we need to check ACL of bucket
void S3DeleteObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

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

#include <utility>

#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"
#include "s3_motr_kvs_writer.h"
#include "s3_motr_writer.h"
#include "s3_option.h"
#include "s3_probable_delete_record.h"
#include "s3_put_object_action_base.h"
#include "s3_uri_to_motr_oid.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3PutObjectActionBase::S3PutObjectActionBase(
    std::shared_ptr<S3RequestObject> s3_request_object,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(s3_request_object),
                     std::move(bucket_meta_factory),
                     std::move(object_meta_factory)) {

  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_s3_factory) {
    motr_writer_factory = std::move(motr_s3_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }
  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
}

void S3PutObjectActionBase::_set_layout_id(int layout_id) {
  assert(layout_id > 0 && layout_id < 15);
  this->layout_id = layout_id;

  motr_write_payload_size =
      S3Option::get_instance()->get_motr_write_payload_size(layout_id);
  assert(motr_write_payload_size > 0);

  motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layout_id);
  assert(motr_unit_size > 0);
}

void S3PutObjectActionBase::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::validationFailed;

  switch (bucket_metadata->get_state()) {
    case S3BucketMetadataState::missing:
      s3_log(S3_LOG_DEBUG, request_id, "Bucket not found");
      set_s3_error("NoSuchBucket");
      break;
    case S3BucketMetadataState::failed_to_launch:
      s3_log(S3_LOG_ERROR, request_id,
             "Bucket metadata load operation failed due to pre launch failure");
      set_s3_error("ServiceUnavailable");
      break;
    default:
      s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed");
      set_s3_error("InternalError");
  }
  send_response_to_s3_client();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::fetch_additional_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::validationFailed;

  switch (additional_bucket_metadata->get_state()) {
    case S3BucketMetadataState::missing:
      s3_log(S3_LOG_ERROR, request_id, "Bucket: [%s] not found",
             additional_bucket_name.c_str());
      set_s3_error("NoSuchBucket");
      break;
    case S3BucketMetadataState::failed_to_launch:
      s3_log(S3_LOG_ERROR, request_id,
             "Bucket metadata load operation failed due to pre launch failure");
      set_s3_error("ServiceUnavailable");
      break;
    default:
      s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed");
      set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectActionBase::fetch_ext_object_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auto omds = additional_object_metadata->get_state();
  if (omds == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_ERROR, request_id,
           "Extended object md not found for object [%s]\n",
           object_metadata->get_object_name().c_str());
    set_s3_error("InternalError");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Metadata load state %d\n", (int)omds);
    if (omds == S3ObjectMetadataState::failed_to_launch) {
      set_s3_error("ServiceUnavailable");
    } else {
      set_s3_error("InternalError");
    }
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit\n", __func__);
}

void S3PutObjectActionBase::fetch_additional_object_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::validationFailed;

  const auto& additional_object_list_layout =
      additional_bucket_metadata->get_object_list_index_layout();

  if (zero(additional_object_list_layout.oid)) {
    s3_log(S3_LOG_ERROR, request_id, "Object not found\n");
    set_s3_error("NoSuchKey");
  } else {
    switch (additional_object_metadata->get_state()) {
      case S3ObjectMetadataState::missing:
        set_s3_error("NoSuchKey");
        break;
      case S3ObjectMetadataState::failed_to_launch:
        s3_log(S3_LOG_ERROR, request_id,
               "Additional object metadata load operation failed due to pre "
               "launch "
               "failure\n");
        set_s3_error("ServiceUnavailable");
        break;
      default:
        s3_log(S3_LOG_DEBUG, request_id,
               "Additional object metadata fetch failed\n");
        set_s3_error("InternalError");
    }
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectActionBase::fetch_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  const auto metadata_state = object_metadata->get_state();

  if (metadata_state == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Destination object is absent");
  } else if (metadata_state == S3ObjectMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Destination object already exists");

    old_object_oid = object_metadata->get_oid();
    old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
    old_layout_id = object_metadata->get_layout_id();

    create_new_oid(old_object_oid);
    // TODO: Read extended object's parts/fragments, depending on the type of
    // primary object.
    // If object is extended, create S3ObjectExtendedMetadata and load extended
    // entries.
    if (object_metadata->is_object_extended()) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Destination object is multipart/fragmented");
      // Read the extended parts of the object from extended index table
      std::shared_ptr<S3ObjectExtendedMetadata> extended_obj_metadata =
          object_metadata_factory->create_object_ext_metadata_obj(
              request, request->get_bucket_name(), request->get_object_name(),
              object_metadata->get_obj_version_key(),
              object_metadata->get_number_of_parts(),
              object_metadata->get_number_of_fragments(),
              bucket_metadata->get_extended_metadata_index_layout());
      object_metadata->set_extended_object_metadata(extended_obj_metadata);
      extended_obj_metadata->load(
          std::bind(&S3PutObjectActionBase::fetch_old_ext_object_info_success,
                    this),
          std::bind(&S3PutObjectActionBase::fetch_ext_object_info_failed,
                    this));
      return;
    }
  } else {
    s3_put_action_state = S3PutObjectActionState::validationFailed;

    if (metadata_state == S3ObjectMetadataState::failed_to_launch) {
      s3_log(S3_LOG_ERROR, request_id,
             "Object metadata load operation failed due to pre launch failure");
      set_s3_error("ServiceUnavailable");
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Failed to look up metadata.");
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
    return;
  }
  // Check if additioanl metadata needs to be loaded,
  // case1: CopyObject API
  std::string source = request->get_headers_copysource();
  if (!source.empty()) {  // this is CopyObject API request
    get_source_bucket_and_object(source);
    fetch_additional_bucket_info();
  } else {
    // No additional metadata load required.
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::fetch_old_ext_object_info_success() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // Check if additioanl metadata needs to be loaded,
  // case1: CopyObject API
  std::string source = request->get_headers_copysource();
  if (!source.empty()) {  // this is CopyObject API request
    get_source_bucket_and_object(source);
    fetch_additional_bucket_info();
  } else {
    // No additional metadata load required.
    next();
  }
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit\n", __func__);
}

void S3PutObjectActionBase::fetch_object_info_failed() {
  // Proceed to to next task, object metadata doesnt exist, will create now

  if (zero(obj_list_idx_lo.oid) || zero(obj_version_list_idx_lo.oid)) {
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
    return;
  }
  // Check if additioanl metadata needs to be loaded,
  // case1: CopyObject API
  std::string source = request->get_headers_copysource();
  if (!source.empty()) {  // this is CopyObject API request
    get_source_bucket_and_object(source);
    fetch_additional_bucket_info();
  } else {
    // No additional metadata load required.
    next();
  }
}

void S3PutObjectActionBase::create_objects() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);
  if (additional_object_metadata->get_number_of_parts() == 0) {
    // FNO + base object itself as fragment
    total_objects = additional_object_metadata->get_number_of_fragments() + 1;
    // TODO -- Handle fragmented objects
  } else {
    // Object is multipart (may be fragmented) then total object =
    // fragments(FNO)
    total_objects = additional_object_metadata->get_number_of_fragments();
    is_multipart = true;
  }
  s3_log(S3_LOG_DEBUG, request_id,
         "Total fragements/parts to be copied : (%d)\n", total_objects);
  extended_obj = additional_object_metadata->get_extended_object_metadata();
  const std::map<int, std::vector<struct s3_part_frag_context>>& ext_entries =
      extended_obj->get_raw_extended_entries();
  if (!is_multipart) {
    // Handle for non multipart during fault tolerance
    // TODO
  } else {
    for (int i = 0; i < total_objects; i++) {
      if (!is_multipart) {
        // Handle for non multipart during fault tolerance
        // TODO
      } else {
        // multipart (may be fragmented) object
        // For now, ext_entry_index is always 0,
        // assuming multipart is not fragmented.
        int ext_entry_index = 0;

        const struct s3_part_frag_context& frag_info =
            (ext_entries.at(i + 1)).at(ext_entry_index);
        part_fragment_context_list.push_back(frag_info);
      }
    }
  }
  create_parts_fragments(0);
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3PutObjectActionBase::create_parts_fragments(int index) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  motr_writer = motr_writer_factory->create_motr_writer(request);
  motr_writer_list.push_back(motr_writer);
  new_ext_oid = {0ULL, 0ULL};
  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_ext_oid);

  _set_layout_id(part_fragment_context_list[index].layout_id);

  motr_writer->create_object(
      std::bind(&S3PutObjectActionBase::create_part_fragment_successful, this),
      std::bind(&S3PutObjectActionBase::create_part_fragment_failed, this),
      new_ext_oid, part_fragment_context_list[index].layout_id);
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_part_fragment_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // TODO -- set status for each index
  s3_put_action_state = S3PutObjectActionState::newObjOidCreated;

  // New Object or overwrite, create new metadata and release old.
  if (new_object_metadata == nullptr) {
    new_object_metadata = object_metadata_factory->create_object_metadata_obj(
        request, bucket_metadata->get_object_list_index_layout(),
        bucket_metadata->get_objects_version_list_index_layout());
    new_object_metadata->regenerate_version_id();
    new_object_metadata->set_oid(new_object_oid);
    // Create extended metadata object and add parts/fragements to it.
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_object_metadata =
        new_object_metadata->get_extended_object_metadata();
    if (!ext_object_metadata) {
      std::shared_ptr<S3ObjectExtendedMetadata> new_ext_object_metadata =
          object_metadata_factory->create_object_ext_metadata_obj(
              request, object_metadata->get_bucket_name(),
              object_metadata->get_object_name(),
              new_object_metadata->get_obj_version_key(), 0, 0,
              bucket_metadata->get_extended_metadata_index_layout());

      new_object_metadata->set_extended_object_metadata(
          new_ext_object_metadata);
      s3_log(S3_LOG_DEBUG, stripped_request_id,
             "Created extended object metadata to store object parts");
    }
    // new_object_metadata->set_oid(motr_writer->get_oid());
    // new_object_metadata->set_layout_id(layout_id);
    // new_object_metadata->set_pvid(motr_writer->get_ppvid());
  }

  const std::shared_ptr<S3ObjectExtendedMetadata>& new_object_ext_metadata =
      new_object_metadata->get_extended_object_metadata();

  new_ext_oid_str = S3M0Uint128Helper::to_string(new_ext_oid);

  if (new_object_ext_metadata) {
    struct s3_part_frag_context new_part_frag_ctx = {};
    new_part_frag_ctx.motr_OID = motr_writer->get_oid();
    memcpy(&new_part_frag_ctx.PVID, motr_writer->get_ppvid(),
           sizeof(struct m0_fid));
    new_part_frag_ctx.versionID = new_object_metadata->get_obj_version_key();
    new_part_frag_ctx.item_size = part_fragment_context_list[index].item_size;

    // Get max part size of all parts
    if (max_part_size == 0) {
      max_part_size = new_part_frag_ctx.item_size;
    } else {
      if (new_part_frag_ctx.item_size > max_part_size) {
        max_part_size = new_part_frag_ctx.item_size;
      }
    }
    new_part_frag_ctx.layout_id = part_fragment_context_list[index].layout_id;
    new_part_frag_ctx.part_number =
        part_fragment_context_list[index].part_number;
    new_part_frag_ctx.is_multipart =
        part_fragment_context_list[index].is_multipart;
    // TODO - remove part_number, as its in part_frag_ctx ?
    new_object_ext_metadata->add_extended_entry(new_part_frag_ctx, 1,
                                                new_part_frag_ctx.part_number);
    object_metadata->set_number_of_fragments(
        new_object_ext_metadata->get_fragment_count());
    object_metadata->set_number_of_parts(
        new_object_ext_metadata->get_part_count());
    new_part_frag_ctx_list.push_back(new_part_frag_ctx);
    add_parts_fragment_oid_to_probable_dead_oid_list(new_object_metadata,
                                                     new_part_frag_ctx);
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_part_fragment_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (!tried_count) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }
  _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
      total_data_to_stream));

  motr_writer->create_object(
      std::bind(&S3PutObjectActionBase::create_object_successful, this),
      std::bind(&S3PutObjectActionBase::create_object_failed, this),
      new_object_oid, layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::newObjOidCreated;

  // New Object or overwrite, create new metadata and release old.
  new_object_metadata = object_metadata_factory->create_object_metadata_obj(
      request, bucket_metadata->get_object_list_index_layout(),
      bucket_metadata->get_objects_version_list_index_layout());

  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  // Generate a version id for the new object.
  // new_object_metadata->regenerate_version_id();
  new_object_metadata->set_oid(motr_writer->get_oid());
  new_object_metadata->set_layout_id(layout_id);
  new_object_metadata->set_pvid(motr_writer->get_ppvid());

  add_object_oid_to_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::exists) {
    collision_detected();
  } else {
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

void S3PutObjectActionBase::collision_detected() {
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, NULL, "Shutdown or rollback");

  } else if (tried_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id, "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid(new_object_oid);
    ++tried_count;
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
  s3_log(S3_LOG_DEBUG, NULL, "Exiting");
}

void S3PutObjectActionBase::create_new_oid(struct m0_uint128 current_oid) {
  unsigned salt_counter = 0;
  std::string salted_uri;

  do {
    salted_uri = request->get_object_uri() + "uri_salt_" +
                 std::to_string(salt_counter) + std::to_string(tried_count);
    S3UriToMotrOID(s3_motr_api, salted_uri.c_str(), request_id,
                   &new_object_oid);
    ++salt_counter;
  } while ((new_object_oid.u_hi == current_oid.u_hi) &&
           (new_object_oid.u_lo == current_oid.u_lo));
}

// Add object parts of specified object to
// the specified probable dead oid list
void S3PutObjectActionBase::add_part_object_to_probable_dead_oid_list(
    const std::shared_ptr<S3ObjectMetadata>& object_metadata,
    std::vector<std::unique_ptr<S3ProbableDeleteRecord>>&
        parts_probable_del_rec_list) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  const std::shared_ptr<S3ObjectExtendedMetadata>& object_ext_metadata =
      object_metadata->get_extended_object_metadata();
  unsigned int total_parts =
      object_ext_metadata ? object_ext_metadata->get_part_count() : 0;
  if (total_parts) {
    s3_log(S3_LOG_DEBUG, request_id, "Total parts available [%u]\n",
           total_parts);
    // object has some parts
    const std::map<int, std::vector<struct s3_part_frag_context>>&
        ext_entries_mp = object_ext_metadata->get_raw_extended_entries();

    std::map<int, std::vector<struct s3_part_frag_context>>::const_iterator it =
        ext_entries_mp.begin();

    while (it != ext_entries_mp.end()) {
      const std::vector<struct s3_part_frag_context>& part_entry = it->second;
      // Assuming part is not fragmented, the only part is at part_entry[0]
      if (part_entry.size() != 0 &&
          (part_entry[0].motr_OID.u_hi || part_entry[0].motr_OID.u_lo)) {
        std::string ext_oid_str =
            S3M0Uint128Helper::to_string(part_entry[0].motr_OID);
        // prepending a char depending on the size of the object (size based
        // bucketing of object)
        S3CommonUtilities::size_based_bucketing_of_objects(
            ext_oid_str, part_entry[0].item_size);

        old_obj_oids.push_back(part_entry[0].motr_OID);
        old_obj_pvids.push_back(part_entry[0].PVID);
        old_obj_layout_ids.push_back(part_entry[0].layout_id);

        s3_log(S3_LOG_DEBUG, request_id,
               "Adding part object, with oid [%" SCNx64 " : %" SCNx64
               "]"
               ", key [%s] to probable delete record\n",
               part_entry[0].motr_OID.u_hi, part_entry[0].motr_OID.u_lo,
               ext_oid_str.c_str());
        std::string pvid_str;
        S3M0Uint128Helper::to_string(part_entry[0].PVID, pvid_str);
        std::unique_ptr<S3ProbableDeleteRecord> ext_del_rec;
        ext_del_rec.reset(new S3ProbableDeleteRecord(
            ext_oid_str, {0ULL, 0ULL}, object_metadata->get_object_name(),
            part_entry[0].motr_OID, part_entry[0].layout_id, pvid_str,
            bucket_metadata->get_object_list_index_layout().oid,
            bucket_metadata->get_objects_version_list_index_layout().oid,
            object_metadata->get_version_key_in_index(),
            false /* force_delete */, false, {0ULL, 0ULL}, 1, it->first,
            bucket_metadata->get_extended_metadata_index_layout().oid,
            part_entry[0].versionID, object_metadata->get_oid()));
        parts_probable_del_rec_list.push_back(std::move(ext_del_rec));
        ++it;
      }
    }  // End of For
    // Add one more entry for parent multipart object so that S3 BD can erase
    // it from the version index, once it is marked for deletion
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
        false, {0ULL, 0ULL}, 1, total_parts /* Total parts in parent object */,
        bucket_metadata->get_extended_metadata_index_layout().oid,
        object_metadata->get_obj_version_key(), {0ULL, 0ULL}));
    parts_probable_del_rec_list.push_back(std::move(probable_del_rec));
  }
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit\n", __func__);
}

void S3PutObjectActionBase::add_object_oid_to_probable_dead_oid_list(
    bool only_old_object) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi || old_object_oid.u_lo) {
    assert(!old_oid_str.empty());
    if (object_metadata->is_object_extended()) {
      // This object was uploaded previously in multipart fashion
      add_part_object_to_probable_dead_oid_list(object_metadata,
                                                probable_del_rec_list);
      for (auto& probable_rec : probable_del_rec_list) {
        probable_oid_list[probable_rec->get_key()] = probable_rec->to_json();
      }
    } else {
      // Simple object
      // prepending a char depending on the size of the object (size based
      // bucketing of object)
      S3CommonUtilities::size_based_bucketing_of_objects(
          old_oid_str, object_metadata->get_content_length());

      // key = oldoid + "-" + newoid
      std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
      s3_log(S3_LOG_DEBUG, request_id,
             "Adding old_probable_del_rec with key [%s]\n",
             old_oid_rec_key.c_str());
      old_probable_del_rec.reset(new S3ProbableDeleteRecord(
          old_oid_rec_key, {0, 0}, object_metadata->get_object_name(),
          old_object_oid, old_layout_id, object_metadata->get_pvid_str(),
          bucket_metadata->get_object_list_index_layout().oid,
          bucket_metadata->get_objects_version_list_index_layout().oid,
          object_metadata->get_version_key_in_index(),
          false /* force_delete */));

      probable_oid_list[old_oid_rec_key] = old_probable_del_rec->to_json();
      probable_del_rec_list.push_back(std::move(old_probable_del_rec));
      old_obj_oids.push_back(old_object_oid);
      old_obj_pvids.push_back(object_metadata->get_pvid());
      old_obj_layout_ids.push_back(old_layout_id);
    }
  }
  if (!only_old_object) {
    std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding new_probable_del_rec with key [%s]\n", new_oid_str.c_str());
    new_probable_del_rec.reset(new S3ProbableDeleteRecord(
        new_oid_str, old_object_oid, new_object_metadata->get_object_name(),
        new_object_oid, layout_id, new_object_metadata->get_pvid_str(),
        bucket_metadata->get_object_list_index_layout().oid,
        bucket_metadata->get_objects_version_list_index_layout().oid,
        new_object_metadata->get_version_key_in_index(),
        false /* force_delete */));

    // store new oid, key = newoid
    probable_oid_list[new_oid_str] = new_probable_del_rec->to_json();
    new_oid_str_list.push_back(new_oid_str);
    new_probable_del_rec_list.push_back(std::move(new_probable_del_rec));
    if (!motr_kv_writer) {
      motr_kv_writer =
          mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
    }
  }
  if (probable_oid_list.size() > 0) {
    motr_kv_writer->put_keyval(
        global_probable_dead_object_list_index_layout, probable_oid_list,
        std::bind(&S3PutObjectActionBase::next, this),
        std::bind(&S3PutObjectActionBase::
                       add_object_oid_to_probable_dead_oid_list_failed,
                  this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::add_parts_fragment_oid_to_probable_dead_oid_list(
    std::shared_ptr<S3ObjectMetadata>& object_metadata,
    struct s3_part_frag_context part_frag_ctx) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_ext_oid_str.empty());

  // Prepending a char depending on the size of the object (size based
  // bucketing of object)
  S3CommonUtilities::size_based_bucketing_of_objects(new_ext_oid_str,
                                                     part_frag_ctx.item_size);

  s3_log(S3_LOG_DEBUG, request_id,
         "Adding new_probable_del_rec for extend with key [%s]\n",
         new_ext_oid_str.c_str());
  std::string pvid_str;
  std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;
  S3M0Uint128Helper::to_string(part_frag_ctx.PVID, pvid_str);
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      new_ext_oid_str, {0ULL, 0ULL}, object_metadata->get_object_name(),
      new_ext_oid, part_frag_ctx.layout_id, pvid_str,
      bucket_metadata->get_object_list_index_layout().oid,
      bucket_metadata->get_objects_version_list_index_layout().oid,
      object_metadata->get_version_key_in_index(), false /* force_delete */,
      part_frag_ctx.is_multipart, {0ULL, 0ULL}, 0, part_frag_ctx.part_number,
      bucket_metadata->get_extended_metadata_index_layout().oid,
      part_frag_ctx.versionID));

  // store new oid, key = newoid
  probable_oid_list[new_ext_oid_str] = new_probable_del_rec->to_json();
  new_oid_str_list.push_back(new_ext_oid_str);
  new_probable_del_rec_list.push_back(std::move(new_probable_del_rec));
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, probable_oid_list,
      std::bind(&S3PutObjectActionBase::
                     add_parts_fragment_oid_to_probable_dead_oid_list_success,
                this),
      std::bind(&S3PutObjectActionBase::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::
    add_parts_fragment_oid_to_probable_dead_oid_list_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (++index == total_objects) {
    // Add only old object, if any, to probable delete list
    add_object_oid_to_probable_dead_oid_list(true);
    return;
  } else {
    // Create object for another part/fragment
    create_parts_fragments(index);
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::add_object_oid_to_probable_dead_oid_list_failed() {
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

void S3PutObjectActionBase::startcleanup() {
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
      ACTION_TASK_ADD(S3PutObjectActionBase::mark_old_oid_for_deletion, this);
    }
    // remove new oid from probable delete list.
    ACTION_TASK_ADD(S3PutObjectActionBase::remove_new_oid_probable_record,
                    this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // Object overwrite case, old object exists, delete it.
      ACTION_TASK_ADD(S3PutObjectActionBase::delete_old_object, this);
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
    ACTION_TASK_ADD(S3PutObjectActionBase::mark_new_oid_for_deletion, this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // remove old oid from probable delete list.
      ACTION_TASK_ADD(S3PutObjectActionBase::remove_old_oid_probable_record,
                      this);
    }
    ACTION_TASK_ADD(S3PutObjectActionBase::delete_new_object, this);
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

void S3PutObjectActionBase::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!new_oid_str.empty());

  if (new_probable_del_rec_list.size() == 0) {
    next();
    return;
  }

  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : new_probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    std::string rec_key = delete_rec->get_key();
    oid_key_val_mp[rec_key] = delete_rec->to_json();
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             oid_key_val_mp,
                             std::bind(&S3PutObjectActionBase::next, this),
                             std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());
#if 0
  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  // update old oid, force_del = true
  old_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             old_oid_rec_key, old_probable_del_rec->to_json(),
                             std::bind(&S3PutObjectActionBase::next, this),
                             std::bind(&S3PutObjectActionBase::next, this));
#endif
  if (probable_del_rec_list.size() == 0) {
    next();
    return;
  }

  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    std::string old_oid_rec_key = delete_rec->get_key();
    oid_key_val_mp[old_oid_rec_key] = delete_rec->to_json();
  }
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             oid_key_val_mp,
                             std::bind(&S3PutObjectActionBase::next, this),
                             std::bind(&S3PutObjectActionBase::next, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());
#if 0
  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_layout,
                                old_oid_rec_key,
                                std::bind(&S3PutObjectActionBase::next, this),
                                std::bind(&S3PutObjectActionBase::next, this));
#endif
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
                                std::bind(&S3PutObjectActionBase::next, this),
                                std::bind(&S3PutObjectActionBase::next, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!new_oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_layout,
                                new_oid_str_list,
                                std::bind(&S3PutObjectActionBase::next, this),
                                std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::delete_old_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // If PUT is success, we delete old object if present
  assert(old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL);
  if (S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    // Call next task in the pipeline
    next();
    return;
  }
  size_t object_count = old_obj_oids.size();
  if (object_count > 0) {
    assert(old_obj_oids.size() == old_obj_layout_ids.size());
    assert(old_obj_oids.size() == old_obj_pvids.size());
    struct m0_uint128 old_oid = old_obj_oids.back();
    struct m0_fid pvid = old_obj_pvids.back();
    int layout_id = old_obj_layout_ids.back();
    motr_writer->set_oid(old_oid);
    motr_writer->delete_object(
        std::bind(&S3PutObjectActionBase::delete_old_object_success, this),
        std::bind(&S3PutObjectActionBase::next, this), old_oid, layout_id,
        pvid);
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::delete_old_object_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  size_t object_count = old_obj_oids.size();
  if (object_count == 0) {
    // There is no OID within the list
    next();
    return;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Deleted old part object oid "
         "[%" SCNx64 " : %" SCNx64 "]",
         (old_obj_oids.back()).u_hi, (old_obj_oids.back()).u_lo);
  old_obj_oids.pop_back();
  old_obj_pvids.pop_back();
  old_obj_layout_ids.pop_back();

  if (old_obj_oids.size() > 0) {
    delete_old_object();
  } else {
    // Finally remove version metadata
    remove_old_object_version_metadata();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutObjectActionBase::remove_old_object_version_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  object_metadata->remove_version_metadata(
      std::bind(&S3PutObjectActionBase::remove_old_oid_probable_record, this),
      std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::delete_new_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // If PUT failed, then clean new object.
  assert(s3_put_action_state != S3PutObjectActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);
  // TODO -- There can be multiple objects in case of multipart copy
  // Handle it
  motr_writer->delete_object(
      std::bind(&S3PutObjectActionBase::remove_new_oid_probable_record, this),
      std::bind(&S3PutObjectActionBase::next, this), new_object_oid, layout_id,
      new_object_metadata->get_pvid());

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

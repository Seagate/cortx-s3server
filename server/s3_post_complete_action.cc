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

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <unistd.h>

#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_md5_hash.h"
#include "s3_post_complete_action.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_m0_uint128_helper.h"
#include "s3_common_utilities.h"
#include "s3_option.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3PostCompleteAction::S3PostCompleteAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory), false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  upload_id = request->get_query_string_value("uploadId");
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Complete Multipart Upload. Bucket[%s] Object[%s]\
         for UploadId[%s]\n",
         bucket_name.c_str(), object_name.c_str(), upload_id.c_str());

  action_uses_cleanup = true;
  s3_post_complete_action_state = S3PostCompleteActionState::empty;
  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_kvs_reader_factory) {
    s3_motr_kvs_reader_factory = std::move(motr_kvs_reader_factory);
  } else {
    s3_motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
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
  if (motr_s3_writer_factory) {
    motr_writer_factory = std::move(motr_s3_writer_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  object_size = 0;
  current_parts_size = 0;
  prev_fetched_parts_size = 0;
  obj_metadata_updated = false;
  validated_parts_count = 0;
  set_abort_multipart(false);
  count_we_requested = S3Option::get_instance()->get_motr_idx_fetch_count();
  setup_steps();
}

std::string S3PostCompleteAction::generate_etag() {
  s3_log(S3_LOG_DEBUG, request_id, "Generating etag...\n");

  S3AwsEtag awsetag;
  for (std::pair<unsigned int, std::string> p_md5 : part_etags) {
    s3_log(S3_LOG_DEBUG, request_id, "part num [%u] -> etag [%s]\n",
           p_md5.first, p_md5.second.c_str());
    awsetag.add_part_etag(p_md5.second);
  }

  std::string etg = awsetag.finalize();
  s3_log(S3_LOG_DEBUG, request_id, "Resulting etag [%s]\n", etg.c_str());
  return etg;
}

void S3PostCompleteAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  ACTION_TASK_ADD(S3PostCompleteAction::load_and_validate_request, this);
  ACTION_TASK_ADD(S3PostCompleteAction::fetch_multipart_info, this);
  ACTION_TASK_ADD(S3PostCompleteAction::get_next_parts_info, this);
  ACTION_TASK_ADD(
      S3PostCompleteAction::add_object_oid_to_probable_dead_oid_list, this);
  ACTION_TASK_ADD(S3PostCompleteAction::save_metadata, this);
  ACTION_TASK_ADD(S3PostCompleteAction::delete_multipart_metadata, this);
  ACTION_TASK_ADD(S3PostCompleteAction::delete_part_list_index, this);
  ACTION_TASK_ADD(S3PostCompleteAction::send_response_to_s3_client, this);
  // ...
}

void S3PostCompleteAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_post_complete_action_state = S3PostCompleteActionState::validationFailed;
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

void S3PostCompleteAction::fetch_object_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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

void S3PostCompleteAction::fetch_ext_object_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auto omds = object_metadata->get_state();
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

void S3PostCompleteAction::load_and_validate_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (request->get_data_length() > 0) {
    if (request->has_all_body_content()) {
      if (validate_request_body(request->get_full_body_content_as_string())) {
        next();
      } else {
        invalid_request = true;
        if (get_s3_error_code().empty()) {
          set_s3_error("MalformedXML");
        }
        s3_post_complete_action_state =
            S3PostCompleteActionState::validationFailed;
        send_response_to_s3_client();
        return;
      }
    } else {
      request->listen_for_incoming_data(
          std::bind(&S3PostCompleteAction::consume_incoming_content, this),
          request->get_data_length() /* we ask for all */
          );
    }
  } else {
    invalid_request = true;
    set_s3_error("MalformedXML");
    s3_post_complete_action_state = S3PostCompleteActionState::validationFailed;
    send_response_to_s3_client();
    return;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::consume_incoming_content() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (request->is_s3_client_read_error()) {
    s3_post_complete_action_state = S3PostCompleteActionState::validationFailed;
    client_read_error();
  } else if (request->has_all_body_content()) {
    if (validate_request_body(request->get_full_body_content_as_string())) {
      next();
    } else {
      invalid_request = true;
      set_s3_error("MalformedXML");
      s3_post_complete_action_state =
          S3PostCompleteActionState::validationFailed;
      send_response_to_s3_client();
      return;
    }
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::fetch_multipart_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  multipart_index_layout = bucket_metadata->get_multipart_index_layout();
  multipart_metadata =
      object_mp_metadata_factory->create_object_mp_metadata_obj(
          request, multipart_index_layout, upload_id);

  // Loads specific object entry from BUCKET/<Bucket Name>/Multipart index which
  // has inprogress uploads list
  multipart_metadata->load(
      std::bind(&S3PostCompleteAction::fetch_multipart_info_success, this),
      std::bind(&S3PostCompleteAction::fetch_multipart_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::fetch_multipart_info_success() {
  // Set the appropriate members after loading multipart metadata
  old_object_oid = multipart_metadata->get_old_oid();
  old_layout_id = multipart_metadata->get_old_layout_id();
  new_object_oid = multipart_metadata->get_oid();
  layout_id = multipart_metadata->get_layout_id();
  new_pvid = multipart_metadata->get_pvid();

  if (old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL) {
    old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
  }
  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  next();
}

void S3PostCompleteAction::fetch_multipart_info_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Multipart info missing\n");
  s3_post_complete_action_state = S3PostCompleteActionState::validationFailed;
  if (multipart_metadata->get_state() == S3ObjectMetadataState::missing) {
    set_s3_error("InvalidObjectState");
  } else if (multipart_metadata->get_state() ==
             S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Object metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void S3PostCompleteAction::get_next_parts_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id, "Fetching parts list from KV store\n");
  motr_kv_reader =
      s3_motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  motr_kv_reader->next_keyval(
      multipart_metadata->get_part_index_layout(), last_key, count_we_requested,
      std::bind(&S3PostCompleteAction::get_next_parts_info_successful, this),
      std::bind(&S3PostCompleteAction::get_next_parts_info_failed, this));
}

void S3PostCompleteAction::get_next_parts_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with size %d while requested %d\n", __func__,
         (int)motr_kv_reader->get_key_values().size(), (int)count_we_requested);
  if (motr_kv_reader->get_key_values().size() > 0) {
    // Do validation of parts
    if (!validate_parts()) {
      s3_log(S3_LOG_DEBUG, "", "validate_parts failed");
      return;
    }
  }

  if (is_abort_multipart()) {
    s3_log(S3_LOG_DEBUG, request_id, "aborting multipart");
    next();
  } else {
    if (motr_kv_reader->get_key_values().size() < count_we_requested) {
      // Fetched all parts
      validated_parts_count += motr_kv_reader->get_key_values().size();
      if ((parts.size() != 0) ||
          (validated_parts_count != std::stoul(total_parts))) {
        s3_log(S3_LOG_DEBUG, request_id,
               "invalid: parts.size %d validated %d exp %d", (int)parts.size(),
               (int)validated_parts_count, (int)std::stoul(total_parts));
        if (part_metadata) {
          part_metadata->set_state(S3PartMetadataState::missing_partially);
        }
        set_s3_error("InvalidPart");
        s3_post_complete_action_state =
            S3PostCompleteActionState::validationFailed;
        send_response_to_s3_client();
        return;
      }
      // All parts info processed and validated, finalize etag and move ahead.
      s3_log(S3_LOG_DEBUG, request_id, "finalizing");
      etag = generate_etag();
      next();
    } else {
      // Continue fetching
      validated_parts_count += count_we_requested;
      if (!motr_kv_reader->get_key_values().empty()) {
        last_key = motr_kv_reader->get_key_values().rbegin()->first;
      }
      s3_log(S3_LOG_DEBUG, request_id, "continue fetching with %s",
             last_key.c_str());
      get_next_parts_info();
    }
  }
}

void S3PostCompleteAction::get_next_parts_info_failed() {
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    // There may not be any records left
    if ((parts.size() != 0) ||
        (validated_parts_count != std::stoul(total_parts))) {
      if (part_metadata) {
        part_metadata->set_state(S3PartMetadataState::missing_partially);
      }
      set_s3_error("InvalidPart");
      s3_post_complete_action_state =
          S3PostCompleteActionState::validationFailed;
      send_response_to_s3_client();
      return;
    }
    etag = generate_etag();
    next();
  } else {
    if (motr_kv_reader->get_state() ==
        S3MotrKVSReaderOpState::failed_to_launch) {
      s3_log(S3_LOG_ERROR, request_id,
             "Parts metadata next keyval operation failed due to pre launch "
             "failure\n");
      set_s3_error("ServiceUnavailable");
    } else {
      set_s3_error("InternalError");
    }
    s3_post_complete_action_state = S3PostCompleteActionState::validationFailed;
    send_response_to_s3_client();
  }
}

void S3PostCompleteAction::set_abort_multipart(bool abortit) {
  delete_multipart_object = abortit;
}

bool S3PostCompleteAction::is_abort_multipart() {
  // If this returns true it means that request is bad (say parts (apart from
  // last part) are not of same size) so we continue further in cleanup mode.
  // We dont save metadata and delete object/indexes.
  // Its set to true by set_abort_multipart()
  return delete_multipart_object;
}

bool S3PostCompleteAction::validate_parts() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata == NULL) {
    part_metadata = part_metadata_factory->create_part_metadata_obj(
        request, multipart_metadata->get_part_index_layout(), upload_id, 0);
  }
  const auto& part_index_layout = multipart_metadata->get_part_index_layout();
  const auto& parts_batch_from_kvs = motr_kv_reader->get_key_values();

  for (auto part_kv = parts.begin(); part_kv != parts.end();) {
    auto store_kv = parts_batch_from_kvs.find(part_kv->first);
    if (store_kv == parts_batch_from_kvs.end()) {
      // The part from complete request not in current kvs part list
      ++part_kv;
      continue;
    } else {
      s3_log(S3_LOG_DEBUG, request_id, "Metadata for key [%s] -> [%s]\n",
             store_kv->first.c_str(), store_kv->second.second.c_str());
      if (part_metadata->from_json(store_kv->second.second) != 0) {
        s3_log(S3_LOG_ERROR, request_id,
               "Json Parsing failed. Index oid = "
               "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
               part_index_layout.oid.u_hi, part_index_layout.oid.u_lo,
               store_kv->first.c_str(), store_kv->second.second.c_str());
        s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED,
               S3_IEM_METADATA_CORRUPTED_STR, S3_IEM_METADATA_CORRUPTED_JSON);

        // part metadata is corrupted, send response and return from here
        part_metadata->set_state(S3PartMetadataState::missing_partially);
        set_s3_error("InvalidPart");
        s3_post_complete_action_state =
            S3PostCompleteActionState::validationFailed;
        send_response_to_s3_client();
        return false;
      }
      s3_log(S3_LOG_DEBUG, request_id, "Processing Part [%s]\n",
             part_metadata->get_part_number().c_str());

      current_parts_size = part_metadata->get_content_length();
      if (current_parts_size > MAXIMUM_ALLOWED_PART_SIZE) {
        s3_log(S3_LOG_ERROR, request_id,
               "The part %s size(%zu) is larger than max "
               "part size allowed:5GB\n",
               store_kv->first.c_str(), current_parts_size);
        set_s3_error("EntityTooLarge");
        s3_post_complete_action_state =
            S3PostCompleteActionState::validationFailed;
        // set_abort_multipart(true);
        send_response_to_s3_client();
        return false;
      }
      if (current_parts_size < MINIMUM_ALLOWED_PART_SIZE &&
          store_kv->first != total_parts) {
        s3_log(S3_LOG_ERROR, request_id,
               "The part %s size(%zu) is smaller than minimum "
               "part size allowed:%u\n",
               store_kv->first.c_str(), current_parts_size,
               MINIMUM_ALLOWED_PART_SIZE);
        set_s3_error("EntityTooSmall");
        s3_post_complete_action_state =
            S3PostCompleteActionState::validationFailed;
        send_response_to_s3_client();
        return false;
      }
      unsigned int pnum = std::stoul(store_kv->first.c_str());
      object_size += part_metadata->get_content_length();
      part_etags[pnum] = part_metadata->get_md5();
      // Remove the entry from parts map, so that in next
      // validate_parts() we dont have to scan it again
      part_kv = parts.erase(part_kv);
      // Add part object to new object's extended metadata potion
      add_part_object_to_object_extended(part_metadata, new_object_metadata);
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return true;
}

void S3PostCompleteAction::add_part_object_to_object_extended(
    const std::shared_ptr<S3PartMetadata>& part_metadata,
    std::shared_ptr<S3ObjectMetadata>& object_metadata) {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  if (!object_metadata) {
    // Create new object metadata
    object_metadata = object_metadata_factory->create_object_metadata_obj(
        request, bucket_metadata->get_object_list_index_layout());
    object_metadata->set_objects_version_list_index_layout(
        bucket_metadata->get_objects_version_list_index_layout());
    // Dummy oid and layout id for new object
    object_metadata->set_oid(new_object_oid);
    object_metadata->set_layout_id(layout_id);

    // Generate version id for the new obj as it will become live to s3 clients.
    object_metadata->regenerate_version_id();
    // Create extended metadata object and add object parts to it.
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_object_metadata =
        object_metadata->get_extended_object_metadata();
    if (!ext_object_metadata) {
      std::shared_ptr<S3ObjectExtendedMetadata> new_ext_object_metadata =
          object_metadata_factory->create_object_ext_metadata_obj(
              request, request->get_bucket_name(), request->get_object_name(),
              object_metadata->get_obj_version_key(), 0, 0,
              bucket_metadata->get_extended_metadata_index_layout());

      object_metadata->set_extended_object_metadata(new_ext_object_metadata);
      s3_log(S3_LOG_DEBUG, stripped_request_id,
             "Created extended object metadata to store object parts");
    }
  }
  const std::shared_ptr<S3ObjectExtendedMetadata>& new_object_ext_metadata =
      object_metadata->get_extended_object_metadata();

  if (new_object_ext_metadata) {
    // Add object part to new object's extended metadata
    struct s3_part_frag_context part_frag_ctx = {};
    part_frag_ctx.motr_OID = part_metadata->get_oid();
    part_frag_ctx.PVID = part_metadata->get_pvid();
    part_frag_ctx.versionID = object_metadata->get_obj_version_key();
    part_frag_ctx.item_size = part_metadata->get_content_length();
    part_frag_ctx.layout_id = part_metadata->get_layout_id();
    part_frag_ctx.is_multipart = true;
    const int part_number =
        strtoul(part_metadata->get_part_number().c_str(), NULL, 0);
    // For multipart object, pass the part_no as object part number
    // and fragment_no=1 (part is not further fragmented)
    new_object_ext_metadata->add_extended_entry(part_frag_ctx, 1, part_number);
    object_metadata->set_number_of_fragments(
        new_object_ext_metadata->get_fragment_count());
    object_metadata->set_number_of_parts(
        new_object_ext_metadata->get_part_count());
    s3_log(S3_LOG_DEBUG, request_id,
           "Added object part [%d] with object oid"
           "[%" SCNx64 " : %" SCNx64 "] to extended md\n",
           part_number, part_frag_ctx.motr_OID.u_hi,
           part_frag_ctx.motr_OID.u_lo);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Add object parts of specified object (old or new) to
// the specified probable dead oid list
void S3PostCompleteAction::add_part_object_to_probable_dead_oid_list(
    const std::shared_ptr<S3ObjectMetadata>& object_metadata,
    std::vector<std::unique_ptr<S3ProbableDeleteRecord>>&
        parts_probable_del_rec_list,
    bool is_old_object) {
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
    struct m0_uint128 old_oid, part_list_index_oid;
    // bool is_multipart = false;
    for (unsigned int key_indx = 1; key_indx <= total_parts; key_indx++) {
      const std::vector<struct s3_part_frag_context>& part_entry =
          ext_entries_mp.at(key_indx);
      // Assuming part is not fragmented, the only part is at part_entry[0]
      if (part_entry.size() != 0 &&
          (part_entry[0].motr_OID.u_hi || part_entry[0].motr_OID.u_lo)) {
        std::string ext_oid_str =
            S3M0Uint128Helper::to_string(part_entry[0].motr_OID);
        // prepending a char depending on the size of the object (size based
        // bucketing of object)
        S3CommonUtilities::size_based_bucketing_of_objects(
            ext_oid_str, part_entry[0].item_size);
        if (is_old_object) {
          // key = oldoid + "-" + newoid; // (newoid = dummy oid of new object)
          ext_oid_str = ext_oid_str + '-' + new_oid_str;
          old_oid = {0ULL, 0ULL};
          part_list_index_oid = {0ULL, 0ULL};
        } else {
          // is_multipart = true;
          part_list_index_oid = multipart_metadata->get_part_index_layout().oid;
          old_oid = old_object_oid;
        }

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
            ext_oid_str, old_oid, object_metadata->get_object_name(),
            part_entry[0].motr_OID, part_entry[0].layout_id, pvid_str,
            bucket_metadata->get_object_list_index_layout().oid,
            bucket_metadata->get_objects_version_list_index_layout().oid,
            object_metadata->get_version_key_in_index(),
            false /* force_delete */, part_entry[0].is_multipart,
            part_list_index_oid, 1, key_indx,
            bucket_metadata->get_extended_metadata_index_layout().oid,
            part_entry[0].versionID));
        parts_probable_del_rec_list.push_back(std::move(ext_del_rec));
      }
    }  // End of For
  }
}

void S3PostCompleteAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  new_object_metadata->set_oid(new_object_oid);
  new_object_metadata->set_layout_id(layout_id);
  new_object_metadata->set_pvid(&new_pvid);
  // Generate version id for the new obj as it will become live to s3 clients.
  new_object_metadata->regenerate_version_id();

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  if (is_abort_multipart()) {
    // Mark new object for probable deletion, we delete obj only after object
    // metadata is deleted.
    assert(!new_oid_str.empty());
    add_part_object_to_probable_dead_oid_list(new_object_metadata,
                                              new_parts_probable_del_rec_list);
    std::map<std::string, std::string> kv_list;
    for (auto& probable_rec : new_parts_probable_del_rec_list) {
      kv_list[probable_rec->get_key()] = probable_rec->to_json();
      new_obj_oids.push_back(probable_rec->get_current_object_oid());
    }
    if (!kv_list.empty()) {
      // S3 Background delete will delete new object parts, when multipart
      // metadata has been deleted
      motr_kv_writer->put_keyval(
          global_probable_dead_object_list_index_layout, kv_list,
          std::bind(&S3PostCompleteAction::
                         add_object_oid_to_probable_dead_oid_list_success,
                    this),
          std::bind(&S3PostCompleteAction::
                         add_object_oid_to_probable_dead_oid_list_failed,
                    this));
    } else {
      next();
    }
  } else {
    // TODO add probable part list index delete record.
    // Mark old object if any for probable deletion.
    // store old object oid
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      assert(!old_oid_str.empty());
      assert(!new_oid_str.empty());
      add_part_object_to_probable_dead_oid_list(
          object_metadata, old_parts_probable_del_rec_list, true);
      std::map<std::string, std::string> kv_list;
      for (auto& probable_rec : old_parts_probable_del_rec_list) {
        kv_list[probable_rec->get_key()] = probable_rec->to_json();
        old_obj_oids.push_back(probable_rec->get_current_object_oid());
      }
      if (!kv_list.empty()) {
        // S3 Background delete will delete old object parts, when current
        // object metadata has
        // moved on
        motr_kv_writer->put_keyval(
            global_probable_dead_object_list_index_layout, kv_list,
            std::bind(&S3PostCompleteAction::
                           add_object_oid_to_probable_dead_oid_list_success,
                      this),
            std::bind(&S3PostCompleteAction::
                           add_object_oid_to_probable_dead_oid_list_failed,
                      this));
      } else {
        next();
      }
    } else {
      // Not an overwrite case, complete multipart upload for brand new object
      next();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::add_object_oid_to_probable_dead_oid_list_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_post_complete_action_state =
      S3PostCompleteActionState::probableEntryRecordSaved;
  next();
  s3_log(S3_LOG_INFO, "", "%s Exit", __func__);
}

void S3PostCompleteAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_post_complete_action_state =
      S3PostCompleteActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::save_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (is_abort_multipart()) {
    next();
  } else {
    for (auto it : multipart_metadata->get_user_attributes()) {
      new_object_metadata->add_user_defined_attribute(it.first, it.second);
    }
    // to rest Date and Last-Modfied time object metadata
    new_object_metadata->reset_date_time_to_current();
    new_object_metadata->set_tags(multipart_metadata->get_tags());
    new_object_metadata->set_content_length(std::to_string(object_size));
    new_object_metadata->set_content_type(
        multipart_metadata->get_content_type());
    new_object_metadata->set_md5(etag);
    new_object_metadata->set_pvid_str(multipart_metadata->get_pvid_str());

    new_object_metadata->save(
        std::bind(&S3PostCompleteAction::save_object_metadata_succesful, this),
        std::bind(&S3PostCompleteAction::save_object_metadata_failed, this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::save_object_metadata_succesful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  obj_metadata_updated = true;
  s3_post_complete_action_state = S3PostCompleteActionState::metadataSaved;
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::save_object_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_post_complete_action_state = S3PostCompleteActionState::metadataSaveFailed;
  if (new_object_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_multipart_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  multipart_metadata->remove(
      std::bind(&S3PostCompleteAction::delete_multipart_metadata_success, this),
      std::bind(&S3PostCompleteAction::delete_multipart_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_multipart_metadata_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (is_abort_multipart()) {
    s3_post_complete_action_state =
        S3PostCompleteActionState::abortedSinceValidationFailed;
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO - mark this for cleanup by backgrounddelete on failure??
void S3PostCompleteAction::delete_multipart_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // S3 backgrounddelete should delete KV from multi part index
  s3_log(S3_LOG_ERROR, request_id,
         "Deletion of %s key failed from multipart index\n",
         object_name.c_str());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO - mark this for cleanup by backgrounddelete on failure??
void S3PostCompleteAction::delete_part_list_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  part_metadata->remove_index(
      std::bind(&S3PostCompleteAction::next, this),
      std::bind(&S3PostCompleteAction::delete_part_list_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_part_list_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  const auto& part_index_layout = part_metadata->get_part_index_layout();
  // S3 backgrounddelete should cleanup/remove part index
  s3_log(S3_LOG_ERROR, request_id,
         "Deletion of part index failed, this oid will be stale in Motr"
         "%" SCNx64 " : %" SCNx64 "\n",
         part_index_layout.oid.u_hi, part_index_layout.oid.u_lo);
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

bool S3PostCompleteAction::validate_request_body(std::string& xml_str) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  xmlNode* child_node;
  xmlChar* xml_part_number;
  xmlChar* xml_etag;
  std::string partnumber = "";
  std::string prev_partnumber = "";
  int previous_part;
  std::string input_etag;

  s3_log(S3_LOG_DEBUG, request_id, "xml string = %s", xml_str.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar*)xml_str.c_str());
  if (document == NULL) {
    xmlFreeDoc(document);
    s3_log(S3_LOG_ERROR, request_id, "The xml string %s is invalid\n",
           xml_str.c_str());
    return false;
  }

  /*Get the root element node */
  xmlNodePtr root_node = xmlDocGetRootElement(document);

  // xmlNodePtr child = root_node->xmlChildrenNode;
  xmlNodePtr child = root_node->xmlChildrenNode;
  while (child != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Xml Tag = %s\n", (char*)child->name);
    if (!xmlStrcmp(child->name, (const xmlChar*)"Part")) {
      partnumber = "";
      input_etag = "";
      for (child_node = child->children; child_node != NULL;
           child_node = child_node->next) {
        if ((!xmlStrcmp(child_node->name, (const xmlChar*)"PartNumber"))) {
          xml_part_number = xmlNodeGetContent(child_node);
          if (xml_part_number != NULL) {
            partnumber = (char*)xml_part_number;
            xmlFree(xml_part_number);
            xml_part_number = NULL;
          }
        }
        if ((!xmlStrcmp(child_node->name, (const xmlChar*)"ETag"))) {
          xml_etag = xmlNodeGetContent(child_node);
          if (xml_etag != NULL) {
            input_etag = (char*)xml_etag;
            xmlFree(xml_etag);
            xml_etag = NULL;
          }
        }
      }
      if (!partnumber.empty() && !input_etag.empty()) {
        parts[partnumber] = input_etag;
        if (prev_partnumber.empty()) {
          previous_part = 0;
        } else {
          previous_part = std::stoi(prev_partnumber);
        }
        if (previous_part > std::stoi(partnumber)) {
          // The request doesn't contain part numbers in ascending order
          set_s3_error("InvalidPartOrder");
          xmlFreeDoc(document);
          s3_log(S3_LOG_DEBUG, request_id,
                 "The XML string doesn't contain parts in ascending order\n");
          return false;
        }
        prev_partnumber = partnumber;
      } else {
        s3_log(S3_LOG_DEBUG, request_id,
               "Error: Part number/Etag missing for a part\n");
        xmlFreeDoc(document);
        return false;
      }
    }
    child = child->next;
  }
  if (partnumber == "") {
    s3_log(S3_LOG_ERROR, request_id,
           "The xml string %s doesn't contain parts\n", xml_str.c_str());
    xmlFreeDoc(document);
    return false;
  }
  total_parts = partnumber;
  xmlFreeDoc(document);
  return true;
}

void S3PostCompleteAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (is_error_state() && !get_s3_error_code().empty()) {
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
  } else if (is_abort_multipart()) {
    S3Error error("InvalidObjectState", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (obj_metadata_updated == true) {
    std::string response;
    std::string object_name = request->get_object_name();
    std::string bucket_name = request->get_bucket_name();
    std::string object_uri = request->get_object_uri();
    response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    response += "<CompleteMultipartUploadResult xmlns=\"http://s3\">\n";
    response += "<Location>" + object_uri + "</Location>";
    response += "<Bucket>" + bucket_name + "</Bucket>\n";
    response += "<Key>" + object_name + "</Key>\n";
    response += "<ETag>\"" + etag + "\"</ETag>";
    response += "</CompleteMultipartUploadResult>";

    s3_post_complete_action_state = S3PostCompleteActionState::completed;
    request->send_response(S3HttpSuccess200, response);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  }
  request->resume(false);
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::startcleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;

  if (s3_post_complete_action_state == S3PostCompleteActionState::empty ||
      s3_post_complete_action_state ==
          S3PostCompleteActionState::validationFailed ||
      s3_post_complete_action_state ==
          S3PostCompleteActionState::probableEntryRecordFailed) {
    // Nothing to undo.
    done();
    return;
  } else if (s3_post_complete_action_state ==
             S3PostCompleteActionState::completed) {
    // New object has taken life, old object should be deleted if any.
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // mark old OID for deletion in overwrite case, this optimizes
      // backgrounddelete decisions.
      ACTION_TASK_ADD(S3PostCompleteAction::mark_old_oid_for_deletion, this);
      ACTION_TASK_ADD(S3PostCompleteAction::delete_old_object, this);
    }
    ACTION_TASK_ADD(S3PostCompleteAction::remove_new_oid_probable_record, this);
  } else if (s3_post_complete_action_state ==
             S3PostCompleteActionState::abortedSinceValidationFailed) {
    // Abort is due to validation failures in part sizes 1..n
    ACTION_TASK_ADD(S3PostCompleteAction::mark_new_oid_for_deletion, this);
    ACTION_TASK_ADD(S3PostCompleteAction::remove_old_oid_probable_record, this);
    ACTION_TASK_ADD(S3PostCompleteAction::delete_new_object, this);
  } else {
    // Any other failure/states we dont clean up objects as next S3 client
    // action will decide
    if (s3_post_complete_action_state ==
            S3PostCompleteActionState::probableEntryRecordSaved ||
        s3_post_complete_action_state ==
            S3PostCompleteActionState::metadataSaved ||
        s3_post_complete_action_state ==
            S3PostCompleteActionState::metadataSaveFailed ||
        s3_post_complete_action_state == S3PostCompleteActionState::completed) {
      ACTION_TASK_ADD(S3PostCompleteAction::remove_new_oid_probable_record,
                      this);
    } else {
      // Should never be here.
      assert(false);
      s3_log(S3_LOG_ERROR, request_id,
             "Possible bug: s3_post_complete_action_state[%d]\n",
             s3_post_complete_action_state);
    }
  }
  // Start running the cleanup task list
  start();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  if (old_parts_probable_del_rec_list.size() == 0) {
    next();
    return;
  }

  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : old_parts_probable_del_rec_list) {
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
                             std::bind(&S3PostCompleteAction::next, this),
                             std::bind(&S3PostCompleteAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_old_object() {

  if (!motr_writer) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }

  // process to delete old object
  assert(old_object_oid.u_hi || old_object_oid.u_lo);

  // If old object exists and deletion of old is disabled, then return
  if ((old_object_oid.u_hi || old_object_oid.u_lo) &&
      S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Skipping deletion of old object. The old object will be deleted by "
           "BD.\n");
    // Call next task in the pipeline
    next();
    return;
  }
  size_t object_count = old_obj_oids.size();
  if (object_count > 0) {
    struct m0_uint128 old_oid = old_obj_oids.back();
    motr_writer->set_oid(old_oid);
    motr_writer->delete_object(
        std::bind(&S3PostCompleteAction::delete_old_object_success, this),
        std::bind(&S3PostCompleteAction::next, this), old_oid, layout_id,
        object_metadata->get_pvid());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_old_object_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  size_t object_count = old_obj_oids.size();
  if (object_count == 0) {
    next();
    return;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Deleted old part object oid "
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

void S3PostCompleteAction::remove_old_object_version_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_metadata) {
    assert(multipart_metadata->get_object_name() == request->get_object_name());
    object_metadata->set_oid(old_object_oid);
    object_metadata->set_layout_id(old_layout_id);
    object_metadata->set_version_id(
        multipart_metadata->get_old_obj_version_id());
    object_metadata->remove_version_metadata(
        std::bind(&S3PostCompleteAction::remove_old_fragments, this),
        std::bind(&S3PostCompleteAction::remove_old_fragments, this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Delete entries corresponding to old objects
// from extended md index
void S3PostCompleteAction::remove_old_fragments() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_metadata && object_metadata->is_object_extended()) {
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_metadata =
        object_metadata->get_extended_object_metadata();
    if (ext_metadata->has_entries()) {
      // Delete fragments, if any
      ext_metadata->remove(
          std::bind(&S3PostCompleteAction::remove_old_ext_metadata_successful,
                    this),
          std::bind(&S3PostCompleteAction::remove_old_ext_metadata_successful,
                    this));
    } else {
      // Extended object exist, but no fragments, delete probbale index entries
      remove_old_oid_probable_record();
    }
  } else {
    // If this is not extended object, delete entries from probable index
    remove_old_oid_probable_record();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::remove_old_ext_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id,
         "Removed extended metadata for Object [%s].\n",
         (object_metadata->get_object_name()).c_str());
  remove_old_oid_probable_record();
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
}

void S3PostCompleteAction::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  if (old_parts_probable_del_rec_list.size() == 0) {
    next();
    return;
  }
  std::vector<std::string> keys_to_delete;
  for (auto& probable_rec : old_parts_probable_del_rec_list) {
    std::string old_oid_rec_key = probable_rec->get_key();
    keys_to_delete.push_back(old_oid_rec_key);
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_layout,
                                keys_to_delete,
                                std::bind(&S3PostCompleteAction::next, this),
                                std::bind(&S3PostCompleteAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());
  assert(is_abort_multipart());

  if (new_parts_probable_del_rec_list.size() == 0) {
    next();
    return;
  }

  std::map<std::string, std::string> oid_key_val_mp;
  for (auto& delete_rec : new_parts_probable_del_rec_list) {
    // force_del = true
    delete_rec->set_force_delete(true);
    std::string new_oid_rec_key = delete_rec->get_key();
    oid_key_val_mp[new_oid_rec_key] = delete_rec->to_json();
  }
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             oid_key_val_mp,
                             std::bind(&S3PostCompleteAction::next, this),
                             std::bind(&S3PostCompleteAction::next, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_new_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(new_object_oid.u_hi || new_object_oid.u_lo);
  assert(is_abort_multipart());

  if (!motr_writer) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }
  unsigned int object_count = new_obj_oids.size();
  if (object_count > 0) {
    struct m0_uint128 new_oid = new_obj_oids.back();
    motr_writer->set_oid(new_oid);
    motr_writer->delete_object(
        std::bind(&S3PostCompleteAction::delete_new_object_success, this),
        std::bind(&S3PostCompleteAction::next, this), new_oid, layout_id,
        multipart_metadata->get_pvid());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::delete_new_object_success() {
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
    // All created objects have been deleted. Try to delete fragment
    // entries from extended metadata index, if any
    remove_new_fragments();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::remove_new_fragments() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (new_object_metadata && new_object_metadata->is_object_extended()) {
    const std::shared_ptr<S3ObjectExtendedMetadata>& ext_metadata =
        new_object_metadata->get_extended_object_metadata();
    if (ext_metadata->has_entries()) {
      // Delete fragments, if any
      ext_metadata->remove(
          std::bind(&S3PostCompleteAction::remove_new_ext_metadata_successful,
                    this),
          std::bind(&S3PostCompleteAction::remove_new_ext_metadata_successful,
                    this));
    } else {
      // Extended object exist, but no fragments, delete probbale index entries
      remove_new_oid_probable_record();
    }
  } else {
    // If this is not extended object, delete entries from probable index
    remove_new_oid_probable_record();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}
void S3PostCompleteAction::remove_new_ext_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id,
         "Removed extended metadata for Object [%s].\n",
         (new_object_metadata->get_object_name()).c_str());
  remove_new_oid_probable_record();
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
}

void S3PostCompleteAction::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  if (new_parts_probable_del_rec_list.size() == 0) {
    next();
    return;
  }
  std::vector<std::string> keys_to_delete;
  for (auto& probable_rec : new_parts_probable_del_rec_list) {
    keys_to_delete.push_back(probable_rec->get_key());
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_layout,
                                keys_to_delete,
                                std::bind(&S3PostCompleteAction::next, this),
                                std::bind(&S3PostCompleteAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PostCompleteAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

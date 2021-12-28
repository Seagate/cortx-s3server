/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#include <string>
#include <evhttp.h>

#include "s3_error_codes.h"
#include "s3_get_bucket_action.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"
#include "s3_object_metadata.h"
#include "s3_common_utilities.h"
#include "s3_list_object_versions_action.h"

#define MAX_RETRY_COUNT 5

S3ListObjectVersionsAction::S3ListObjectVersionsAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory)
    : S3BucketAction(req, bucket_meta_factory),
      fetch_successful(false),
      key_count(0),
      response_is_truncated(false),
      total_keys_visited(0),
      encoding_type(req->get_query_string_value("encoding-type")) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  s3_log(S3_LOG_INFO, stripped_request_id, "S3 API: List Object Versions.\n");

  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_kvs_reader_factory) {
    s3_motr_kvs_reader_factory = motr_kvs_reader_factory;
  } else {
    s3_motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }
  if (object_meta_factory) {
    object_metadata_factory = object_meta_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }
  include_marker_in_result = true;
  // When we skip keys with same common prefix, we need a way to indicate
  // a state in which we'll make use of existing key fetch logic just to see if
  // any more keys are available in bucket after the keys with common prefix.
  // This is required to return correct truncation flag in the response.
  check_any_keys_after_prefix = false;
  setup_steps();
}

S3ListObjectVersionsAction::~S3ListObjectVersionsAction() {}

void S3ListObjectVersionsAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3ListObjectVersionsAction::validate_request, this);
  ACTION_TASK_ADD(S3ListObjectVersionsAction::get_next_versions, this);
  ACTION_TASK_ADD(S3ListObjectVersionsAction::check_latest_versions, this);
  ACTION_TASK_ADD(S3ListObjectVersionsAction::send_response_to_s3_client, this);
}

void S3ListObjectVersionsAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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

void S3ListObjectVersionsAction::validate_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  bool send_response = false;
  // Fetch input parameters
  bucket_name = request->get_bucket_name();
  s3_log(S3_LOG_DEBUG, request_id, "bucket name = %s\n", bucket_name.c_str());
  request_prefix = request->get_query_string_value("prefix");
  s3_log(S3_LOG_DEBUG, request_id, "prefix = %s\n", request_prefix.c_str());
  request_delimiter = request->get_query_string_value("delimiter");
  s3_log(S3_LOG_DEBUG, request_id, "delimiter = %s\n",
         request_delimiter.c_str());
  request_key_marker = request->get_query_string_value("key-marker");
  s3_log(S3_LOG_DEBUG, request_id, "request_key_marker = %s\n",
         request_key_marker.c_str());
  last_key = request_key_marker;  // As requested by user

  // Validate the input max-keys
  std::string max_k = request->get_query_string_value("max-keys");
  if (max_k.empty()) {
    max_keys = 1000;
  } else {
    int max_keys_temp = 0;
    if ((!S3CommonUtilities::stoi(max_k, max_keys_temp)) ||
        (max_keys_temp < 0)) {
      s3_log(S3_LOG_DEBUG, request_id, "invalid max-keys = %s\n",
             max_k.c_str());
      // TODO: Add invalid argument details to the response.
      set_s3_error("InvalidArgument");
      send_response = true;
    } else {
      max_keys = max_keys_temp;
    }
  }

  // Validate the input encoding-type
  if (request->has_query_param_key("encoding-type")) {
    std::string encoding_type =
        request->get_query_string_value("encoding-type");
    if (encoding_type != "url") {
      s3_log(S3_LOG_DEBUG, request_id, "invalid encoding-type = %s\n",
             encoding_type.c_str());
      // TODO: Add invalid argument details to the response.
      set_s3_error("InvalidArgument");
      send_response = true;
    }
  }

  if (send_response) {
      send_response_to_s3_client();
      return;
  }

  s3_log(S3_LOG_DEBUG, request_id, "max-keys = %s\n", max_k.c_str());
  next();
}

void S3ListObjectVersionsAction::get_next_versions() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If client is disconnected (say, due to read timeout, when
  // the list object versions takes substantial time), destroy action class.
  if (!request->client_connected()) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "s3 client is disconnected. Terminating list object versions "
           "request\n");
    S3_RESET_SHUTDOWN_SIGNAL;  // For shutdown testcases
    done();
    return;
  }

  const auto& object_version_list_index_layout =
      bucket_metadata->get_objects_version_list_index_layout();

  if (motr_kv_reader == nullptr) {
    motr_kv_reader = s3_motr_kvs_reader_factory->create_motr_kvs_reader(
        request, s3_motr_api);
  }
  // If keyval read operation failed due rpc message size threshold,
  // try with lesser size.
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::failed_e2big) {
    if (retry_count > MAX_RETRY_COUNT) {
      max_record_count = 1;
    } else {
      max_record_count = max_record_count / 2;
    }
  } else {
    max_record_count = S3Option::get_instance()->get_motr_idx_fetch_count();
  }
  s3_log(S3_LOG_DEBUG, stripped_request_id, "max_record_count set to %zu\n",
         max_record_count);

  if (max_keys == 0 || zero(object_version_list_index_layout.oid)) {
    // As requested max_keys is 0 or version_list is not present
    // Go ahead and respond.
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    // We pass M0_OIF_EXCLUDE_START_KEY flag to Motr. This flag skips key that
    // is passed during listing of all keys. If this flag is not passed then
    // input key is returned in result.
    if (!request_prefix.empty() && request_key_marker.empty() &&
        include_marker_in_result) {
      include_marker_in_result = false;
      last_key = request_prefix;
      motr_kv_reader->next_keyval(
          object_version_list_index_layout, last_key, max_record_count,
          std::bind(&S3ListObjectVersionsAction::get_next_versions_successful,
                    this),
          std::bind(&S3ListObjectVersionsAction::get_next_versions_failed,
                    this),
          0);
    } else {
      motr_kv_reader->next_keyval(
          object_version_list_index_layout, last_key, max_record_count,
          std::bind(&S3ListObjectVersionsAction::get_next_versions_successful,
                    this),
          std::bind(&S3ListObjectVersionsAction::get_next_versions_failed,
                    this));
    }
  }

  // For shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "list_object_versions_get_next_versions_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ListObjectVersionsAction::get_next_versions_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  retry_count = 0;
  s3_log(S3_LOG_DEBUG, request_id, "Found object versions list\n");
  bool break_out = false;
  bool last_key_in_common_prefix = false;
  bool no_further_prefix_match = false;
  bool skip_remaining_common_prefixes = false;
  std::string last_common_prefix;
  auto& kvps = motr_kv_reader->get_key_values();
  size_t length = kvps.size();

  // Check if this is the call to identify any more keys left
  // in bucket, after skipping keys belonging to same common prefix.
  // In this state, we don't check/process each key.
  // We simply return response to client.
  if (check_any_keys_after_prefix) {
    if (length != 0) {
      // There exist keys after last common prefix.
      if (!request_prefix.empty()) {
        // If prefix is specified, we need to make sure that there exists
        // atleast one key with same prefix. If no, it indicates no keys.
        const std::string& first_key = kvps.begin()->first;
        if (first_key.find(request_prefix) == std::string::npos) {
          // There exists no key matching the prefix, set 'length' to 0
          length = 0;
        }
      }
      s3_log(S3_LOG_DEBUG, request_id,
             "Found [%zu] keys after the last common prefix [%s]\n", length,
             saved_last_key.c_str());

      if (length != 0) {
        response_is_truncated = true;
        s3_log(S3_LOG_DEBUG, request_id, "Next marker = %s\n",
               saved_last_key.c_str());
        add_marker_version(saved_last_key, "");
        add_next_marker_version(kvps.begin()->first,
                                kvps.begin()->second.second);
      }
    }
    // Reset state
    check_any_keys_after_prefix = false;
    fetch_successful = true;
    next();
    return;
  }

  // Statistics - Total keys visited/loaded
  if (!kvps.empty()) {
    total_keys_visited += length;
  }

  for (auto& kv : kvps) {
    // Remove /version_id from the key.
    std::string key = kv.first.substr(0, kv.first.find_last_of("/"));
    if (break_out) {
      add_next_marker_version(kv.first, kv.second.second);
      break;
    }
    s3_log(S3_LOG_DEBUG, request_id, "Read object version = %s\n",
           kv.first.c_str());
    s3_log(S3_LOG_DEBUG, request_id, "Read object version value = %s\n",
           kv.second.second.c_str());
    // Check if the current key cannot be rolled into the last common prefix.
    // If can't be rolled into last common prefix then reset
    // 'last_key_in_common_prefix'
    if (last_key_in_common_prefix) {
      // Filter by prefix, if prefix specified
      if (!request_prefix.empty()) {
        // Filter out by prefix
        if (key.find(request_prefix) == std::string::npos) {
          // Key does not start with specified prefix; key filtered out.
          // Prefix does not match.
          // Check if fetched key is lexicographically greater than prefix
          if (key > request_prefix) {
            // No further prefix match will occur (as items in Motr storage are
            // arranaged in lexical order).
            no_further_prefix_match = true;
            // Set length to zero to set truncation to false
            length = 0;
            s3_log(S3_LOG_INFO, stripped_request_id,
                   "No further prefix match. Skipping further object versions "
                   "listing\n");
            break;
          }
          if (--length == 0) {
            break;
          } else {
            // Check the next key
            continue;
          }
        }
      }
      size_t common_prefix_pos = key.find(last_common_prefix);
      if (common_prefix_pos == std::string::npos) {
        // As we didn't find key with same common prefix, it means we have one
        // more key added to the list.
        // Now check if we have reached the max keys requested. If yes, break
        if ((versions_list.size() + common_prefixes.size()) == max_keys) {
          // Before breaking the loop, set the last key to last common prefix
          last_key = last_common_prefix;
          break;
        }
        // We didn't find current key with same last common prefix.
        // Reset 'last_key_in_common_prefix'
        last_key_in_common_prefix = false;
        last_common_prefix = "";
      } else {
        // Continue to next key as current key also rolls into the same
        // last common prefix.
        --length;
        last_key = kv.first;
        continue;
      }
    }

    size_t delimiter_pos = std::string::npos;
    if (request_prefix.empty() && request_delimiter.empty()) {
      add_object_version(kv.first, kv.second.second);
    } else if (!request_prefix.empty() && request_delimiter.empty()) {
      // Filter out by prefix
      if (key.find(request_prefix) == 0) {
        add_object_version(kv.first, kv.second.second);
      } else {
        // Prefix does not match.
        // Check if fetched key is lexicographically greater than prefix
        if (key > request_prefix) {
          // No further prefix match will occur (as items in Motr storage are
          // arranged in lexical order)
          no_further_prefix_match = true;
          // Set length to zero to indicate truncation is false
          length = 0;
          s3_log(S3_LOG_INFO, stripped_request_id,
                 "No further prefix match. Skipping further object versions "
                 "listing\n");
          break;
        }
      }
    } else if (request_prefix.empty() && !request_delimiter.empty()) {
      delimiter_pos = key.find(request_delimiter);
      if (delimiter_pos == std::string::npos) {
        add_object_version(kv.first, kv.second.second);
      } else {
        // Roll up
        // All keys under a common prefix are counted as a single key
        // When we encounter a key that belongs to common prefix, the code
        // maintains a state 'last_key_in_common_prefix', indicating a key
        // with common prefix.
        s3_log(S3_LOG_DEBUG, request_id,
               "Delimiter %s found at pos %zu in string %s\n",
               request_delimiter.c_str(), delimiter_pos, key.c_str());
        std::string common_prefix = key.substr(0, delimiter_pos + 1);
        skip_remaining_common_prefixes = false;
        // Before adding common prefix, check if this is the first time we add
        // this key to common_prefixes. If so, skip remaining keys that belong
        // to it.
        if (!is_prefix_in_common_prefix(common_prefix)) {
          skip_remaining_common_prefixes = true;
        }
        if (common_prefix != request_key_marker) {
          // If marker is specified, and if this key gets rolled up
          // in common prefix, we add to common prefix only if it is not the
          // same as specified marker
          add_common_prefix(common_prefix);
          s3_log(S3_LOG_DEBUG, request_id, "Adding common prefix [%s]\n",
                 common_prefix.c_str());
          last_common_prefix = common_prefix;
          last_key_in_common_prefix = true;
        }
        if (skip_remaining_common_prefixes) {
          // Skip remaining common prefixes
          // For this, set last key for the next key enumeration and break
          // Motr will skip returning the keys with the common prefix.
          last_key = common_prefix + "\xff";
          s3_log(S3_LOG_DEBUG, request_id,
                 "Skipping further common prefixes, set next key = [%s]\n",
                 last_key.c_str());
          break;
        }
      }
    } else {
      // Both prefix and delimiter are not empty
      bool prefix_match = key.find(request_prefix) == 0;
      if (prefix_match) {
        delimiter_pos = key.find(request_delimiter, request_prefix.length());
        if (delimiter_pos == std::string::npos) {
          add_object_version(kv.first, kv.second.second);
        } else {
          // Roll up
          // All keys under a common prefix are counted as a single key
          // When we encounter a key that belongs to common prefix, the code
          // maintains a state 'last_key_in_common_prefix', indicating a key
          // with common prefix.
          s3_log(S3_LOG_DEBUG, request_id,
                 "Delimiter %s found at pos %zu in string %s\n",
                 request_delimiter.c_str(), delimiter_pos, key.c_str());
          std::string common_prefix = key.substr(0, delimiter_pos + 1);
          skip_remaining_common_prefixes = false;
          // Before adding common prefix, check if this is the first time we
          // add this key to common_prefixes.
          // If so, skip remaining keys that belong to it.
          if (!is_prefix_in_common_prefix(common_prefix)) {
            skip_remaining_common_prefixes = true;
          }
          if (common_prefix != request_key_marker) {
            // If marker is specified, and if this key gets rolled up
            // in common prefix, we add to common prefix only if it is not the
            // same as specified marker
            add_common_prefix(common_prefix);
            s3_log(S3_LOG_DEBUG, request_id, "Adding common prefix [%s]\n",
                   common_prefix.c_str());
            last_common_prefix = common_prefix;
            last_key_in_common_prefix = true;
          }
          if (skip_remaining_common_prefixes) {
            // Skip remaining common prefixes
            // For this, set last key for the next key enumeration and break
            // Motr will skip returning the keys with the common prefix.
            last_key = common_prefix + "\xff";
            s3_log(S3_LOG_DEBUG, request_id,
                   "Skipping further common prefixes, set next key = [%s]\n",
                   last_key.c_str());
            break;
          }
        }
      } else {
        // Prefix does not match.
        // Check if fetched key is lexicographically greater than prefix
        if (key > request_prefix) {
          // No further prefix match will occur (as items in Motr storage are
          // arranaged in lexical order)
          no_further_prefix_match = true;
          // Set length to zero to set truncation to false
          length = 0;
          s3_log(S3_LOG_INFO, stripped_request_id,
                 "No further prefix match. Skipping further object versions "
                 "listing\n");
          break;
        }
      }
    }

    if ((--length == 0) ||
        (!last_key_in_common_prefix &&
         ((versions_list.size() + common_prefixes.size()) == max_keys))) {
      // This is the last element returned or we reached limit requested, we
      // break.
      last_key = kv.first;
      break_out = true;
    }
  }  // End of for loop

  if (json_error) {
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
  }

  // We ask for more if there is any.
  key_count = versions_list.size() + common_prefixes.size();
  if ((key_count == max_keys) ||
      (!skip_remaining_common_prefixes && (kvps.size() < max_record_count)) ||
      (no_further_prefix_match)) {
    // Go ahead and respond.
    if (key_count == max_keys && length != 0) {
      // When we hit the max keys condition and previously we skipped common
      // prefix keys (i.e. skip_remaining_common_prefixes = true),
      // we can't rely on 'length' variable for truncation flag.
      // In such case, we don't know if there are any more keys left after
      // skipping the common prefix. In such situation, we have to explictly
      // identify if there are any further keys left in a bucket to mark
      // the truncation flag in the response.
      if (skip_remaining_common_prefixes) {
        // Check with Motr if there are any keys to be seen.
        // If there are still some keys, we would set truncation flag, else no.
        check_any_keys_after_prefix = true;
        if (last_key_in_common_prefix) {
          saved_last_key = last_common_prefix;
        }
        // Call get_next_versions to see remaining keys, if any, after
        // skipping keys belonging to common prefix.
        get_next_versions();
        return;
      } else {
        response_is_truncated = true;
        // Before sending response, check if the previous key was in common
        // prefix. If yes, we need to return the common prefix as next marker.
        if (last_key_in_common_prefix) {
          last_key = last_common_prefix;
          add_marker_version(last_common_prefix, "");
        } else {
          const auto& version = versions_list.back();
          add_marker_version(version->get_object_name(),
                             version->get_obj_version_id());
        }
        s3_log(S3_LOG_DEBUG, request_id, "Next marker = %s\n",
               last_key.c_str());
      }
    }
    fetch_successful = true;
    next();
  } else {
    get_next_versions();
  }
}

void S3ListObjectVersionsAction::get_next_versions_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  switch (motr_kv_reader->get_state()) {
    case S3MotrKVSReaderOpState::missing:
      s3_log(S3_LOG_DEBUG, request_id, "No versions found in listing\n");
      // Reset state
      check_any_keys_after_prefix = false;
      fetch_successful = true;  // With no entries.
      break;
    case S3MotrKVSReaderOpState::failed_to_launch:
      s3_log(S3_LOG_ERROR, request_id,
             "Bucket metadata next keyval operation failed due to pre launch "
             "failure\n");
      set_s3_error("ServiceUnavailable");
      break;
    case S3MotrKVSReaderOpState::failed_e2big:
      s3_log(S3_LOG_ERROR, request_id,
             "Next keyval operation failed due rpc message size threshold\n");
      if (max_record_count <= 1) {
        set_s3_error("InternalError");
        fetch_successful = false;
        s3_log(S3_LOG_ERROR, request_id,
               "Next keyval operation failed due rpc message size threshold, "
               "even with max record retrieval as one\n");
      } else {
        retry_count++;
        s3_log(S3_LOG_INFO, request_id,
               "Retry next key val, retry count = %d\n", retry_count);
        get_next_versions();
        s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
        return;
      }
      break;
    default:
      s3_log(S3_LOG_DEBUG, request_id, "Failed to find versions listing\n");
      set_s3_error("InternalError");
      fetch_successful = false;
      break;
  };
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ListObjectVersionsAction::check_latest_versions() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::shared_ptr<S3ObjectMetadata> version_metadata;
  size_t version_list_offset_temp = 0;
  // Run through version_list, check and set "latest" flag.
  for (auto&& version : versions_list) {
    if (last_object_checked.empty()) {
      version_metadata = version;
      break;
    } else if ((version_list_offset_temp <= version_list_offset) ||
               (last_object_checked == version->get_object_name())) {
      version_list_offset_temp++;
      continue;
    } else {
      version_metadata = version;
      break;
    }
  }
  if (version_metadata) {
    version_list_offset = version_list_offset_temp;
    last_object_checked = version_metadata->get_object_name();
    const auto& object_list_idx_lo =
        bucket_metadata->get_object_list_index_layout();
    const auto& obj_version_list_idx_lo =
        bucket_metadata->get_objects_version_list_index_layout();

    if (zero(object_list_idx_lo.oid) || zero(obj_version_list_idx_lo.oid)) {
      // Object list index and version list index missing.
      get_next_versions_failed();
    } else {
      // Read object metadata from object index.
      object_metadata = object_metadata_factory->create_object_metadata_obj(
          request, bucket_name, version_metadata->get_object_name(),
          object_list_idx_lo, obj_version_list_idx_lo);
      object_metadata->load(
          std::bind(&S3ListObjectVersionsAction::fetch_object_info_success,
                    this),
          std::bind(&S3ListObjectVersionsAction::get_next_versions_failed,
                    this));
    }
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ListObjectVersionsAction::fetch_object_info_success() {
  // Compare the versions and set the latest flag
  const auto& version_metadata = versions_list[version_list_offset];
  if (object_metadata->get_obj_version_id() ==
      version_metadata->get_obj_version_id()) {
    version_metadata->set_latest(true);
  }
  check_latest_versions();
}

void S3ListObjectVersionsAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    s3_log(S3_LOG_DEBUG, request_id, "Sending %s response...\n",
           get_s3_error_code().c_str());
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_bucket_name());
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
  } else if (fetch_successful) {
    std::string& response_xml = get_response_xml();
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_bytes_sent(response_xml.length());
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, request_id, "Object list response_xml = %s\n",
           response_xml.c_str());
    // Total visited/touched keys in the bucket
    s3_log(S3_LOG_INFO, stripped_request_id, "Total keys visited = %zu\n",
           total_keys_visited);
    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ListObjectVersionsAction::add_marker_version(
    const std::string& key, const std::string& version_id) {
  key_marker = key;
  version_id_marker = version_id;
}

void S3ListObjectVersionsAction::add_next_marker_version(
    const std::string& next_key, const std::string& next_value) {
  m0_uint128 object_version_list_index_oid =
      bucket_metadata->get_objects_version_list_index_layout().oid;
  auto next_version =
      object_metadata_factory->create_object_metadata_obj(request);
  if (next_version->from_json(next_value) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
           object_version_list_index_oid.u_hi,
           object_version_list_index_oid.u_lo, next_key.c_str(),
           next_value.c_str());
  } else {
    next_key_marker = next_version->get_object_name();
    next_version_id_marker = next_version->get_obj_version_id();
  }
}

void S3ListObjectVersionsAction::add_object_version(const std::string& key,
                                                    const std::string& value) {
  m0_uint128 object_version_list_index_oid =
      bucket_metadata->get_objects_version_list_index_layout().oid;
  auto version = object_metadata_factory->create_object_metadata_obj(request);
  if (version->from_json(value) != 0) {
    json_error = true;
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
           object_version_list_index_oid.u_hi,
           object_version_list_index_oid.u_lo, key.c_str(), value.c_str());
  } else {
    versions_list.push_back(version);
  }
}

void S3ListObjectVersionsAction::add_common_prefix(
    const std::string& common_prefix) {
  common_prefixes.insert(common_prefix);
}

bool S3ListObjectVersionsAction::is_prefix_in_common_prefix(
    const std::string& prefix) {
  return common_prefixes.find(prefix) != common_prefixes.end();
}

std::string& S3ListObjectVersionsAction::get_response_xml() {
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  response_xml +=
      "<ListVersionsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
  response_xml += S3CommonUtilities::format_xml_string(
      "IsTruncated", (response_is_truncated ? "true" : "false"));

  // Run through versions_list, add to response as version or delete marker.
  for (auto&& version : versions_list) {
    if (!version->is_delete_marker()) {
      response_xml += "<Version>";
      response_xml += S3CommonUtilities::format_xml_string(
          "ETag", version->get_md5(), true);
      response_xml += S3CommonUtilities::format_xml_string(
          "IsLatest", (version->is_latest() ? "true" : "false"));
      response_xml += S3CommonUtilities::format_xml_string(
          "Key", get_encoded_key_value(version->get_object_name()));
      response_xml += S3CommonUtilities::format_xml_string(
          "LastModified", version->get_last_modified_iso());
      response_xml += "<Owner>";
      response_xml += S3CommonUtilities::format_xml_string(
          "DisplayName", version->get_account_name());
      response_xml += S3CommonUtilities::format_xml_string(
          "ID", version->get_canonical_id());
      response_xml += "</Owner>";
      response_xml += S3CommonUtilities::format_xml_string(
          "Size", version->get_content_length_str());
      response_xml += S3CommonUtilities::format_xml_string(
          "StorageClass", version->get_storage_class());
      response_xml += S3CommonUtilities::format_xml_string(
          "VersionId", version->get_obj_version_id());
      response_xml += "</Version>";
    } else {
      response_xml += "<DeleteMarker>";
      response_xml += S3CommonUtilities::format_xml_string(
          "IsLatest", (version->is_latest() ? "true" : "false"));
      response_xml += S3CommonUtilities::format_xml_string(
          "Key", get_encoded_key_value(version->get_object_name()));
      response_xml += S3CommonUtilities::format_xml_string(
          "LastModified", version->get_last_modified_iso());
      response_xml += "<Owner>";
      response_xml += S3CommonUtilities::format_xml_string(
          "DisplayName", version->get_account_name());
      response_xml += S3CommonUtilities::format_xml_string(
          "ID", version->get_canonical_id());
      response_xml += "</Owner>";
      response_xml += S3CommonUtilities::format_xml_string(
          "VersionId", version->get_obj_version_id());
      response_xml += "</DeleteMarker>";
    }
  }

  for (auto&& prefix : common_prefixes) {
    response_xml += "<CommonPrefixes>";
    std::string prefix_no_delimiter = prefix;
    // Remove the delimiter from the end
    prefix_no_delimiter.pop_back();
    std::string encoded_prefix = get_encoded_key_value(prefix_no_delimiter);
    // Add the delimiter at the end
    encoded_prefix += request_delimiter;
    response_xml +=
        S3CommonUtilities::format_xml_string("Prefix", encoded_prefix);
    response_xml += "</CommonPrefixes>";
  }

  response_xml +=
      S3CommonUtilities::format_xml_string("Name", request->get_bucket_name());
  response_xml +=
      S3CommonUtilities::format_xml_string("Prefix", request_prefix.c_str());
  if (!request_delimiter.empty()) {
    response_xml +=
        S3CommonUtilities::format_xml_string("Delimiter", request_delimiter);
  }
  response_xml +=
      S3CommonUtilities::format_xml_string("MaxKeys", std::to_string(max_keys));
  if (response_is_truncated) {
    response_xml += S3CommonUtilities::format_xml_string(
        "KeyMarker", get_encoded_key_value(key_marker));
    response_xml += S3CommonUtilities::format_xml_string("VersionIdMarker",
                                                         version_id_marker);
    response_xml += S3CommonUtilities::format_xml_string(
        "NextKeyMarker", get_encoded_key_value(next_key_marker));
    response_xml += S3CommonUtilities::format_xml_string(
        "NextVersionIdMarker", next_version_id_marker);
  }
  if (encoding_type == "url") {
    response_xml += S3CommonUtilities::format_xml_string("EncodingType", "url");
  }

  response_xml += "</ListVersionsResult>";
  return response_xml;
}

// Encoding type used by S3 to encode object key names in the XML response.
// If encoding-type was specified in request parameter, S3 includes this
// element in the response, and returns encoded key name values in the
// following response elements:
// Delimiter, KeyMarker, Prefix, NextKeyMarker, Key.
std::string S3ListObjectVersionsAction::get_encoded_key_value(
    const std::string& key_value) {
  std::string encoded_key_value;
  if (encoding_type == "url") {
    char* encoded_str = evhttp_uriencode(key_value.c_str(), -1, 0);
    encoded_key_value = encoded_str;
    free(encoded_str);
  } else {
    encoded_key_value = key_value;
  }
  return encoded_key_value;
}
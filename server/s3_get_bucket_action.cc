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

#include <string>

#include "s3_error_codes.h"
#include "s3_get_bucket_action.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_option.h"
#include "s3_common_utilities.h"

#define MAX_RETRY_COUNT 5

S3GetBucketAction::S3GetBucketAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory)
    : S3BucketAction(req, bucket_meta_factory),
      object_list(std::make_shared<S3ObjectListResponse>(
          req->get_query_string_value("encoding-type"))),
      last_key(""),
      fetch_successful(false),
      total_keys_visited(0),
      key_Count(0) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Get Bucket(List Objects).\n");

  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }
  if (motr_kvs_reader_factory) {
    s3_motr_kvs_reader_factory = motr_kvs_reader_factory;
  } else {
    s3_motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (object_meta_factory) {
    object_metadata_factory = object_meta_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }
  motr_kv_reader = nullptr;
  b_first_next_keyval_call = true;
  // When we skip keys with same common prefix, we need a way to indicate
  // a state in which we'll make use of existing key fetch logic just to see if
  // any more keys are available in bucket, after the keys with common prefix.
  // This is required to return correct truncation flag in object list response
  b_state_start_check_any_more_keys = false;
  setup_steps();
  // TODO request param validations
}

S3GetBucketAction::~S3GetBucketAction() {}

void S3GetBucketAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3GetBucketAction::validate_request, this);
  ACTION_TASK_ADD(S3GetBucketAction::get_next_objects, this);
  ACTION_TASK_ADD(S3GetBucketAction::send_response_to_s3_client, this);
  // ...
}
void S3GetBucketAction::fetch_bucket_info_failed() {
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
void S3GetBucketAction::validate_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  object_list->set_bucket_name(request->get_bucket_name());
  request_prefix = request->get_query_string_value("prefix");
  object_list->set_request_prefix(request_prefix);
  s3_log(S3_LOG_DEBUG, request_id, "prefix = %s\n", request_prefix.c_str());

  request_delimiter = request->get_query_string_value("delimiter");
  object_list->set_request_delimiter(request_delimiter);
  s3_log(S3_LOG_DEBUG, request_id, "delimiter = %s\n",
         request_delimiter.c_str());

  request_marker_key = request->get_query_string_value("marker");
  if (!request_marker_key.empty()) {
    object_list->set_request_marker_key(request_marker_key);
  }
  s3_log(S3_LOG_DEBUG, request_id, "request_marker_key = %s\n",
         request_marker_key.c_str());

  last_key = request_marker_key;  // as requested by user
  std::string max_k = request->get_query_string_value("max-keys");
  if (max_k.empty()) {
    max_keys = 1000;
    object_list->set_max_keys("1000");
  } else {
    if (!S3CommonUtilities::stoul(max_k, max_keys)) {
      s3_log(S3_LOG_DEBUG, request_id, "invalid max-keys = %s\n",
             max_k.c_str());
      set_s3_error("InvalidArgument");
      send_response_to_s3_client();
      return;
    }
    object_list->set_max_keys(max_k);
  }
  s3_log(S3_LOG_DEBUG, request_id, "max-keys = %s\n", max_k.c_str());
  after_validate_request();
}

void S3GetBucketAction::after_validate_request() { next(); }

void S3GetBucketAction::get_next_objects() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If client is disconnected (say, due to read timeout,
  // when the object listing takes substantial time), destroy action class
  if (!request->client_connected()) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "s3 client is disconnected. Terminating object listing request\n");
    S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
    done();
    return;
  }

  m0_uint128 object_list_index_oid =
      bucket_metadata->get_object_list_index_oid();
  if (motr_kv_reader == nullptr) {
    motr_kv_reader = s3_motr_kvs_reader_factory->create_motr_kvs_reader(
        request, s3_motr_api);
  }

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

  if (max_keys == 0) {
    // as requested max_keys is 0
    // Go ahead and respond.
    fetch_successful = true;
    object_list->set_key_count(key_Count);
    send_response_to_s3_client();
  } else if (object_list_index_oid.u_hi == 0ULL &&
             object_list_index_oid.u_lo == 0ULL) {
    fetch_successful = true;
    object_list->set_key_count(key_Count);
    send_response_to_s3_client();
  } else {
    // We pass M0_OIF_EXCLUDE_START_KEY flag to Motr. This flag skips key that
    // is passed during listing of all keys. If this flag is not passed then
    // input key is returned in result.
    if (!request_prefix.empty() && request_marker_key.empty() &&
        b_first_next_keyval_call) {
      b_first_next_keyval_call = false;
      last_key = request_prefix;
      motr_kv_reader->next_keyval(
          object_list_index_oid, last_key, max_record_count,
          std::bind(&S3GetBucketAction::get_next_objects_successful, this),
          std::bind(&S3GetBucketAction::get_next_objects_failed, this), 0);
    } else {
      motr_kv_reader->next_keyval(
          object_list_index_oid, last_key, max_record_count,
          std::bind(&S3GetBucketAction::get_next_objects_successful, this),
          std::bind(&S3GetBucketAction::get_next_objects_failed, this));
    }
  }

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "get_bucket_action_get_next_objects_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetBucketAction::get_next_objects_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  retry_count = 0;
  s3_log(S3_LOG_DEBUG, request_id, "Found Object listing\n");
  m0_uint128 object_list_index_oid =
      bucket_metadata->get_object_list_index_oid();
  bool atleast_one_json_error = false;
  bool last_key_in_common_prefix = false;
  bool skip_no_further_prefix_match = false;
  bool b_skip_remaining_common_prefixes = false;
  std::string last_common_prefix = "";
  auto& kvps = motr_kv_reader->get_key_values();
  size_t length = kvps.size();
  if (b_state_start_check_any_more_keys) {
    // Check if this is the call to identify any more keys left
    // in bucket, after skipping keys belonging to same common prefix.
    // In this state, we don't check/process each key.
    // We simply return response to client.
    if (length != 0) {
      // There exist keys after last common prefix.
      if (!request_prefix.empty()) {
        // If prefix is specified, we need to make sure that there exists
        // atleast one key with same prefix. If no, it indicates no keys.
        const std::string& firt_key = kvps.begin()->first;
        if (firt_key.find(request_prefix) == std::string::npos) {
          // There exists no key matching the prefix. Set 'length' 0
          length = 0;
        }
      }
      s3_log(S3_LOG_DEBUG, request_id,
             "Found [%zu] keys after the last common prefix [%s]\n", length,
             saved_last_key.c_str());

      if (length != 0) {
        object_list->set_response_is_truncated(true);
        s3_log(S3_LOG_DEBUG, request_id, "Next marker = %s\n",
               saved_last_key.c_str());
        object_list->set_next_marker_key(saved_last_key);
      }
    }
    // reset state
    b_state_start_check_any_more_keys = false;
    fetch_successful = true;
    object_list->set_key_count(key_Count);
    send_response_to_s3_client();
    return;
  }

  // Statistics - Total keys visited/loaded
  if (!kvps.empty()) {
    total_keys_visited += length;
  }

  for (auto& kv : kvps) {
    s3_log(S3_LOG_DEBUG, request_id, "Read Object = %s\n", kv.first.c_str());
    s3_log(S3_LOG_DEBUG, request_id, "Read Object Value = %s\n",
           kv.second.second.c_str());
    // Check if the current key cannot be rolled into the last common prefix.
    // If can't be rolled into last common prefix, reset
    // 'last_key_in_common_prefix'
    if (last_key_in_common_prefix) {
      // Filter by prefix, if prefix specified
      if (!request_prefix.empty()) {
        // Filter out by prefix
        if (kv.first.find(request_prefix) == std::string::npos) {
          // Key does not start with specified prefix; key filtered out.
          // Prefix does not match.
          // Check if fetched key is lexicographically greater than prefix
          if (kv.first > request_prefix) {
            // No further prefix match will occur (as items in Motr storage are
            // arranaged in lexical order)
            skip_no_further_prefix_match = true;
            // Set length to zero to indicate truncation is false
            length = 0;
            s3_log(
                S3_LOG_INFO, stripped_request_id,
                "No further prefix match. Skipping further object listing\n");
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
      size_t common_prefix_pos = kv.first.find(last_common_prefix);
      if (common_prefix_pos == std::string::npos) {
        // As we didn't find key with same common prefix, it means we have one
        // more key added to the list.
        // Now check if we have reached the max keys requested. If yes, break
        if ((object_list->size() + object_list->common_prefixes_size()) ==
            max_keys) {
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

    auto object = object_metadata_factory->create_object_metadata_obj(request);
    size_t delimiter_pos = std::string::npos;
    if (request_prefix.empty() && request_delimiter.empty()) {
      if (object->from_json(kv.second.second) != 0) {
        atleast_one_json_error = true;
        s3_log(S3_LOG_ERROR, request_id,
               "Json Parsing failed. Index oid = "
               "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
               object_list_index_oid.u_hi, object_list_index_oid.u_lo,
               kv.first.c_str(), kv.second.second.c_str());
      } else {
        object_list->add_object(object);
      }
    } else if (!request_prefix.empty() && request_delimiter.empty()) {
      // Filter out by prefix
      if (kv.first.find(request_prefix) == 0) {
        if (object->from_json(kv.second.second) != 0) {
          atleast_one_json_error = true;
          s3_log(S3_LOG_ERROR, request_id,
                 "Json Parsing failed. Index oid = "
                 "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
                 object_list_index_oid.u_hi, object_list_index_oid.u_lo,
                 kv.first.c_str(), kv.second.second.c_str());
        } else {
          object_list->add_object(object);
        }
      } else {
        // Prefix does not match.
        // Check if fetched key is lexicographically greater than prefix
        if (kv.first > request_prefix) {
          // No further prefix match will occur (as items in Motr storage are
          // arranaged in lexical order)
          skip_no_further_prefix_match = true;
          // Set length to zero to indicate truncation is false
          length = 0;
          s3_log(S3_LOG_INFO, stripped_request_id,
                 "No further prefix match. Skipping further object listing\n");
          break;
        }
      }
    } else if (request_prefix.empty() && !request_delimiter.empty()) {
      delimiter_pos = kv.first.find(request_delimiter);
      if (delimiter_pos == std::string::npos) {
        if (object->from_json(kv.second.second) != 0) {
          atleast_one_json_error = true;
          s3_log(S3_LOG_ERROR, request_id,
                 "Json Parsing failed. Index oid = "
                 "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
                 object_list_index_oid.u_hi, object_list_index_oid.u_lo,
                 kv.first.c_str(), kv.second.second.c_str());
        } else {
          object_list->add_object(object);
        }
      } else {
        // Roll up
        // All keys under a common prefix are counted as a single key
        // When we encounter a key that belongs to common prefix, the code
        // maintains a state
        // 'last_key_in_common_prefix', indicating a key with common prefix.
        s3_log(S3_LOG_DEBUG, request_id,
               "Delimiter %s found at pos %zu in string %s\n",
               request_delimiter.c_str(), delimiter_pos, kv.first.c_str());
        std::string common_prefix = kv.first.substr(0, delimiter_pos + 1);
        b_skip_remaining_common_prefixes = false;
        // Before adding common prefix, check if this is the first time we add
        // this key
        // to common_prefixes. If so, skip remaining keys that belong to it.
        if (!object_list->is_prefix_in_common_prefix(common_prefix)) {
          b_skip_remaining_common_prefixes = true;
        }
        if (common_prefix != request_marker_key) {
          // If marker is specified, and if this key gets rolled up
          // in common prefix, we add to common prefix only if it is not the
          // same as specified marker
          object_list->add_common_prefix(common_prefix);
          s3_log(S3_LOG_DEBUG, request_id, "Adding common prefix [%s]\n",
                 common_prefix.c_str());
          last_common_prefix = common_prefix;
          last_key_in_common_prefix = true;
        }
        if (b_skip_remaining_common_prefixes) {
          // Skip remaining common prefixes
          // For this, set last key for the next key enumeration and break
          last_key = common_prefix + "\xff";
          s3_log(S3_LOG_DEBUG, request_id,
                 "Skipping further common prefixes, set next key = [%s]\n",
                 last_key.c_str());
          break;
        }
      }
    } else {
      // both prefix and delimiter are not empty
      bool prefix_match = (kv.first.find(request_prefix) == 0) ? true : false;
      if (prefix_match) {
        delimiter_pos =
            kv.first.find(request_delimiter, request_prefix.length());
        if (delimiter_pos == std::string::npos) {
          if (object->from_json(kv.second.second) != 0) {
            atleast_one_json_error = true;
            s3_log(S3_LOG_ERROR, request_id.c_str(),
                   "Json Parsing failed. Index oid = "
                   "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
                   object_list_index_oid.u_hi, object_list_index_oid.u_lo,
                   kv.first.c_str(), kv.second.second.c_str());
          } else {
            object_list->add_object(object);
          }
        } else {
          // Roll up
          // All keys under a common prefix are counted as a single key
          // When we encounter a key that belongs to common prefix, the code
          // maintains a state 'last_key_in_common_prefix', indicating a key
          // with common prefix.
          s3_log(S3_LOG_DEBUG, request_id,
                 "Delimiter %s found at pos %zu in string %s\n",
                 request_delimiter.c_str(), delimiter_pos, kv.first.c_str());
          std::string common_prefix = kv.first.substr(0, delimiter_pos + 1);
          b_skip_remaining_common_prefixes = false;
          // Before adding common prefix, check if this is the first time we
          // add this key to common_prefixes.
          // If so, skip remaining keys that belong to it.
          if (!object_list->is_prefix_in_common_prefix(common_prefix)) {
            b_skip_remaining_common_prefixes = true;
          }
          if (common_prefix != request_marker_key) {
            // If marker is specified, and if this key gets rolled up
            // in common prefix, we add to common prefix only if it is not the
            // same as specified marker
            object_list->add_common_prefix(common_prefix);
            s3_log(S3_LOG_DEBUG, request_id, "Adding common prefix [%s]\n",
                   common_prefix.c_str());
            last_common_prefix = common_prefix;
            last_key_in_common_prefix = true;
          }
          if (b_skip_remaining_common_prefixes) {
            // Skip remaining common prefixes
            // For this, set last key for the next key enumeration and break
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
        if (kv.first > request_prefix) {
          // No further prefix match will occur (as items in Motr storage are
          // arranaged in lexical order)
          skip_no_further_prefix_match = true;
          // Set length to zero to indicate truncation is false
          length = 0;
          s3_log(S3_LOG_INFO, stripped_request_id,
                 "No further prefix match. Skipping further object listing\n");
          break;
        }
      }
    }

    if ((--length == 0) ||
        (!last_key_in_common_prefix &&
         ((object_list->size() + object_list->common_prefixes_size()) ==
          max_keys))) {
      // This is the last element returned or we reached limit requested, we
      // break.
      last_key = kv.first;
      break;
    }
  }  // end of For loop

  if (atleast_one_json_error) {
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
  }

  // We ask for more if there is any.
  key_Count = object_list->size() + object_list->common_prefixes_size();
  if ((key_Count == max_keys) ||
      (!b_skip_remaining_common_prefixes && (kvps.size() < max_record_count)) ||
      (skip_no_further_prefix_match)) {
    // Go ahead and respond.
    if (key_Count == max_keys && length != 0) {
      // When we hit the max keys condition and previously we skipped common
      // prefix keys
      // (i.e. b_skip_remaining_common_prefixes = true), we can't rely on
      // 'length' variable for truncation flag.
      // In such case, we don't know if there are any more keys left after
      // skipping the common prefix. In such situation, we have to explictly
      // identify if there are any further keys left in a bucket to mark
      // the truncation flag in the object list response.
      if (b_skip_remaining_common_prefixes) {
        // Check with Motr if there are any keys to be seen.
        // If there are still some keys, we would set truncation flag, else no.
        b_state_start_check_any_more_keys = true;
        if (last_key_in_common_prefix) {
          saved_last_key = last_common_prefix;
        }
        // Call get_next_objects to see remaining keys, if any, after skipping
        // keys belonging to common prefix.
        get_next_objects();
        return;
      } else {
        object_list->set_response_is_truncated(true);
        // Before sending response, check if the previous key was in common
        // prefix
        // If yes, we need to return the common prefix as next marker
        // This is required to fix Ceph S3 test case.
        if (last_key_in_common_prefix) {
          last_key = last_common_prefix;
        }
        s3_log(S3_LOG_DEBUG, request_id, "Next marker = %s\n",
               last_key.c_str());
        object_list->set_next_marker_key(last_key);
      }
    }
    fetch_successful = true;
    object_list->set_key_count(key_Count);
    send_response_to_s3_client();
  } else {
    get_next_objects();
  }
}

void S3GetBucketAction::get_next_objects_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "No Objects found in Object listing\n");
    // reset state
    b_state_start_check_any_more_keys = false;
    fetch_successful = true;  // With no entries.
    object_list->set_key_count(key_Count);
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata next keyval operation failed due to pre launch "
           "failure\n");
    set_s3_error("ServiceUnavailable");
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_e2big) {
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
      s3_log(S3_LOG_INFO, request_id, "Retry next key val, retry count = %d\n",
             retry_count);
      get_next_objects();
      s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
      return;
    }
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to find Object listing\n");
    set_s3_error("InternalError");
    fetch_successful = false;
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetBucketAction::send_response_to_s3_client() {
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
      request->set_out_header_value("Retry-After", "1");
    }
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (fetch_successful) {
    std::string& response_xml = object_list->get_xml(
        request->get_canonical_id(), bucket_metadata->get_owner_id(),
        request->get_user_id());
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

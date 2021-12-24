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

#include <iostream>
#include <json/json.h>

#include "s3_api_handler.h"
#include "s3_data_usage_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_option.h"

extern struct s3_motr_idx_layout data_usage_accounts_index_layout;
#define JSON_OBJECTS_COUNT "objects_count"
#define JSON_BYTES_COUNT "bytes_count"

S3DataUsageAction::S3DataUsageAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory,
    std::shared_ptr<MotrAPI> motr_api)
    : S3Action(req, true, nullptr, true, true), request(req) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_api) {
    motr_kvs_api = std::move(motr_api);
  } else {
    motr_kvs_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (kvs_reader_factory) {
    motr_kvs_reader_factory = std::move(kvs_reader_factory);
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  motr_kvs_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, motr_kvs_api);
  max_records_per_request =
      S3Option::get_instance()->get_motr_idx_fetch_count();
  last_key = "";

  setup_steps();
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Exit", __func__);
}

S3DataUsageAction::~S3DataUsageAction() {
  s3_log(S3_LOG_DEBUG, "", "%s\n", __func__);
}

void S3DataUsageAction::get_data_usage_counters() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  motr_kvs_reader->next_keyval(
      data_usage_accounts_index_layout, last_key, max_records_per_request,
      std::bind(&S3DataUsageAction::get_next_keyval_success, this),
      std::bind(&S3DataUsageAction::get_next_keyval_failure, this));
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Exit", __func__);
}

std::string extract_account_id_from_motr_key(const std::string& motr_key) {
  const char account_id_delimeter = '/';
  auto pos = motr_key.find(account_id_delimeter);
  return (pos != std::string::npos ? motr_key.substr(0, pos) : "");
}

int from_json(std::string json, int64_t* p_objects_count,
              int64_t* p_bytes_count) {
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(json.c_str(), newroot);
  if (!parsingSuccessful) {
    return -1;
  }

  if (!newroot.isMember(JSON_OBJECTS_COUNT) ||
      !newroot.isMember(JSON_BYTES_COUNT)) {
    return -1;
  }

  if (p_objects_count) {
    *p_objects_count = newroot[JSON_OBJECTS_COUNT].asInt64();
  }
  if (p_bytes_count) {
    *p_bytes_count = newroot[JSON_BYTES_COUNT].asInt64();
  }
  return 0;
}

void S3DataUsageAction::get_next_keyval_success() {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  auto& kvps = motr_kvs_reader->get_key_values();
  for (auto& kv : kvps) {
    std::string key = kv.first;
    std::string json = kv.second.second;
    s3_log(S3_LOG_DEBUG, request_id, "Read key = %s\n  value=%s", key.c_str(), json.c_str());
    std::string account_id = extract_account_id_from_motr_key(key);
    if (account_id.empty()) {
      s3_log(
          S3_LOG_ERROR, stripped_request_id,
          "Failed to extract the account ID from key %s. Skipping the record",
          key.c_str());
      continue;
    }
    int64_t bytes_counter;
    if (from_json(json, nullptr, &bytes_counter) != 0) {
      s3_log(S3_LOG_ERROR, stripped_request_id,
             "Failed to extract the counters from json %s. Skipping the record",
             json.c_str());
      continue;
    }

    if (counters.find(account_id) != counters.end()) {
      counters[account_id] = bytes_counter;
    } else {
      counters[account_id] += bytes_counter;
    }
  }

  size_t length = kvps.size();
  if (length < max_records_per_request) {
    // Read all, move forward.
    next();
  } else {
    // Read the next chunk of counter records.
    last_key = kvps.rbegin()->first;
    get_data_usage_counters();
  }
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Exit\n", __func__);
}

void S3DataUsageAction::get_next_keyval_failure() {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  auto reader_state = motr_kvs_reader->get_state();
  if (reader_state == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "No(more) records in data usage index");
    next();
    return;
  } else if (reader_state == S3MotrKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Failed to launch data usage index listing");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "Internal error during data usage index listing");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Exit", __func__);
}

void S3DataUsageAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  ACTION_TASK_ADD(S3DataUsageAction::get_data_usage_counters, this);
  ACTION_TASK_ADD(S3DataUsageAction::send_response_to_s3_client, this);
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Exit", __func__);
}

std::string S3DataUsageAction::create_json_response() {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  // Create response in the following format:
  // ["account_id": {"bytes_count": <number>}, ...]
  Json::Value root(Json::arrayValue);
  for (auto& cnt : counters) {
    Json::Value account;
    Json::Value counters;
    counters[JSON_BYTES_COUNT] = Json::Value((Json::Value::Int64)(cnt.second));
    account[cnt.first] = counters;
    root.append(account);
  }
  Json::FastWriter fastWriter;
  std::string json = fastWriter.write(root);
  s3_log(S3_LOG_DEBUG, "", "%s Exit with ret %s\n", __func__, json.c_str());
  return json;
}

void S3DataUsageAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  std::string json = create_json_response();
  request->send_response(S3HttpSuccess200, json);
  s3_log(S3_LOG_DEBUG, "", "%s Exit\n", __func__);
}
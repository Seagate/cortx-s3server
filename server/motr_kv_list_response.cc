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

#include <json/json.h>
#include <evhttp.h>
#include "motr_kv_list_response.h"
#include "s3_common_utilities.h"
#include "s3_log.h"

MotrKVListResponse::MotrKVListResponse(std::string encoding_type)
    : encoding_type(encoding_type),
      request_prefix(""),
      request_delimiter(""),
      request_marker_key(""),
      max_keys(""),
      response_is_truncated(false),
      next_marker_key("") {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
}

void MotrKVListResponse::set_index_id(std::string name) { index_id = name; }

// Encoding type used by S3 to encode object key names in the XML response.
// If you specify encoding-type request parameter, S3 includes this element in
// the response, and returns encoded key name values in the following response
// elements:
// Delimiter, KeyMarker, Prefix, NextKeyMarker, Key.
std::string MotrKVListResponse::get_response_format_key_value(
    const std::string& key_value) {
  std::string format_key_value;
  if (encoding_type == "url") {
    char* decoded_str = evhttp_uriencode(key_value.c_str(), -1, 1);
    format_key_value = decoded_str;
    free(decoded_str);
  } else {
    format_key_value = key_value;
  }
  return format_key_value;
}

void MotrKVListResponse::set_request_prefix(std::string prefix) {
  request_prefix = get_response_format_key_value(prefix);
}

void MotrKVListResponse::set_request_delimiter(std::string delimiter) {
  request_delimiter = get_response_format_key_value(delimiter);
}

void MotrKVListResponse::set_request_marker_key(std::string marker) {
  request_marker_key = get_response_format_key_value(marker);
}

void MotrKVListResponse::set_max_keys(std::string count) { max_keys = count; }

void MotrKVListResponse::set_response_is_truncated(bool flag) {
  response_is_truncated = flag;
}

void MotrKVListResponse::set_next_marker_key(std::string next) {
  next_marker_key = get_response_format_key_value(next);
}

void MotrKVListResponse::add_kv(const std::string& key,
                                const std::string& value) {
  kv_list[key] = value;
}

unsigned int MotrKVListResponse::size() { return kv_list.size(); }

unsigned int MotrKVListResponse::common_prefixes_size() {
  return common_prefixes.size();
}

void MotrKVListResponse::add_common_prefix(std::string common_prefix) {
  common_prefixes.insert(common_prefix);
}

std::string MotrKVListResponse::as_json() {
  // clang-format off
  Json::Value root;
  root["Index-Id"] = index_id;
  root["Prefix"] = request_prefix;
  root["Delimiter"] = request_delimiter;
  if (encoding_type == "url") {
    root["EncodingType"] = "url";
  }
  root["Marker"] = request_marker_key;
  root["MaxKeys"] = max_keys;
  root["NextMarker"] = next_marker_key;
  root["IsTruncated"] = response_is_truncated ? "true" : "false";

  Json::Value keys_array;
  for (auto&& kv : kv_list) {
    Json::Value key_object;
    key_object["Key"] = kv.first;
    key_object["Value"] = kv.second;
    keys_array.append(key_object);
  }

  for (auto&& prefix : common_prefixes) {
    root["CommonPrefixes"] = prefix;
  }

  root["Keys"] = keys_array;

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}
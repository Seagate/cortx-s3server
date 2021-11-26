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

#pragma once

#ifndef __S3_SERVER_S3_LIST_OBJECT_VERSIONS_ACTION_H__
#define __S3_SERVER_S3_LIST_OBJECT_VERSIONS_ACTION_H__

#include <memory>
#include <vector>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_kvs_reader.h"

class S3ListObjectVersionsAction : public S3BucketAction {
  std::shared_ptr<S3MotrKVSReaderFactory> s3_motr_kvs_reader_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<MotrAPI> s3_motr_api;
  bool include_marker_in_result;
  bool check_any_keys_after_prefix;
  size_t max_record_count;
  bool fetch_successful;
  size_t key_Count;
  short retry_count = 0;
  bool response_is_truncated;
  std::string saved_last_key;
  // Identify total keys visited/touched in object versions listing
  size_t total_keys_visited;
  std::string last_key;  // last key during each iteration
  std::string next_key_marker;
  // Request Input params
  std::string bucket_name;
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_key_marker;
  size_t max_keys;
  std::string encoding_type;

  std::vector<std::shared_ptr<S3ObjectMetadata>> versions_list;
  std::set<std::string> common_prefixes;
  std::string response_xml;

  void set_next_key_marker(std::string next, bool url_encode = true);
  void add_object_version(std::shared_ptr<S3ObjectMetadata> object);
  void add_common_prefix(std::string);
  bool is_prefix_in_common_prefix(std::string& prefix);
  std::string& get_response_xml();
  std::string get_encoded_key_value(const std::string& key_value);

 public:
  S3ListObjectVersionsAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  virtual ~S3ListObjectVersionsAction();
  void setup_steps();
  void fetch_bucket_info_failed();
  // Derived class can override S3 request validation logic
  void validate_request();
  void get_next_versions();
  void get_next_versions_successful();
  void get_next_versions_failed();
  void send_response_to_s3_client();
};

#endif

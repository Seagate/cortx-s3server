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
  bool json_error = false;
  short retry_count = 0;
  bool response_is_truncated;
  std::string saved_last_key;
  // Identify total keys visited/touched in object versions listing
  size_t total_keys_visited;
  std::string last_key;  // last key during each iteration
  std::string last_object_checked;
  size_t version_list_offset = 0;
  std::string key_marker;
  std::string version_id_marker;
  std::string next_key_marker;
  std::string next_version_id_marker;
  // Request Input parameters
  std::string bucket_name;
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_key_marker;
  size_t max_keys;
  std::string encoding_type;

  // Response data
  std::vector<std::shared_ptr<S3ObjectMetadata>> versions_list;
  std::set<std::string> common_prefixes;
  std::string response_xml;
  std::shared_ptr<S3ObjectMetadata> object_metadata;

  // Internal functions
  void add_marker_version(std::string key, std::string version_id);
  void add_next_marker_version(std::string next_key, std::string next_value);
  void add_object_version(std::string key, std::string value);
  void add_common_prefix(std::string common_prefix);
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
  void validate_request();
  void get_next_versions();
  void get_next_versions_successful();
  void get_next_versions_failed();
  void check_latest_versions();
  void fetch_object_info_success();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3ListObjectVersionsTest, Constructor);
  FRIEND_TEST(S3ListObjectVersionsTest, FetchBucketInfo);
  FRIEND_TEST(S3ListObjectVersionsTest, FetchBucketInfoFailedMissing);
  FRIEND_TEST(S3ListObjectVersionsTest, FetchBucketInfoFailedToLaunch);
  FRIEND_TEST(S3ListObjectVersionsTest, FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3ListObjectVersionsTest, ValidateRequestInvalidArgument);
  FRIEND_TEST(S3ListObjectVersionsTest, GetNextVersionsZeroMaxkeys);
  FRIEND_TEST(S3ListObjectVersionsTest, GetNextVersionsZeroOid);
  FRIEND_TEST(S3ListObjectVersionsTest, GetNextVersions);
  FRIEND_TEST(S3ListObjectVersionsTest, GetNextVersionsFailed);
  FRIEND_TEST(S3ListObjectVersionsTest, GetNextVersionsSuccessful);
  FRIEND_TEST(S3ListObjectVersionsTest, GetNextVersionsSuccessfulJsonError);
  FRIEND_TEST(S3ListObjectVersionsTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3ListObjectVersionsTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3ListObjectVersionsTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3ListObjectVersionsTest, SendResponseToClientInternalError);
};

#endif

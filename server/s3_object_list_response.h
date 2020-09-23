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

#ifndef __S3_SERVER_S3_OBJECT_LIST_RESPONSE_H__
#define __S3_SERVER_S3_OBJECT_LIST_RESPONSE_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "s3_object_metadata.h"
#include "s3_part_metadata.h"

class S3ObjectListResponse {
  // value can be url or empty string
 protected:
  std::string encoding_type;

  std::string bucket_name;
  std::string object_name, user_name, user_id, storage_class, upload_id;
  std::string account_name, account_id, canonical_id;
  std::vector<std::shared_ptr<S3ObjectMetadata>> object_list;
  std::map<int, std::shared_ptr<S3PartMetadata>> part_list;

  std::set<std::string> common_prefixes;

  // Generated xml response.
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  std::string request_marker_uploadid;
  std::string max_keys;
  bool response_is_truncated;
  std::string next_marker_key;

  std::string max_uploads;
  std::string max_parts;
  std::string next_marker_uploadid;
  std::string key_count;
  std::string response_xml;

  std::string get_response_format_key_value(const std::string& key_value);

 public:
  S3ObjectListResponse(std::string encoding_type = "");

  void set_bucket_name(std::string name);
  void set_object_name(std::string name);
  void set_user_id(std::string);
  void set_user_name(std::string);
  void set_canonical_id(std::string);
  void set_account_id(std::string);
  void set_account_name(std::string);
  void set_storage_class(std::string);
  void set_upload_id(std::string upload_id);
  void set_request_prefix(std::string prefix);
  void set_request_delimiter(std::string delimiter);
  void set_request_marker_key(std::string marker);
  void set_request_marker_uploadid(std::string marker);
  void set_max_keys(std::string count);
  void set_key_count(size_t& keys);
  void set_max_uploads(std::string count);
  void set_max_parts(std::string count);
  void set_response_is_truncated(bool flag);
  void set_next_marker_key(std::string next);
  void set_next_marker_uploadid(std::string next);
  std::string& get_object_name();

  void add_object(std::shared_ptr<S3ObjectMetadata> object);
  void add_part(std::shared_ptr<S3PartMetadata> part);
  void add_common_prefix(std::string);
  unsigned int size();
  unsigned int common_prefixes_size();

  virtual std::string& get_xml(const std::string requestor_canonical_id,
                               const std::string bucket_owner_user_id,
                               const std::string requestor_user_id);
  std::string& get_multipart_xml();
  std::string& get_multiupload_xml();
  std::string& get_user_id();
  std::string& get_user_name();
  std::string& get_canonical_id();
  std::string& get_account_id();
  std::string& get_account_name();
  std::string& get_storage_class();
  std::string& get_upload_id();
  std::string& get_request_delimiter() { return request_delimiter; }
  std::set<std::string>& get_common_prefixes() { return common_prefixes; }
  std::string get_next_marker_key();

  // Google tests.
  FRIEND_TEST(S3GetMultipartPartActionTest, ConstructorTest);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetKeyObjectSuccessfulValueNotEmptyListSizeSameAsMaxAllowed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetKeyObjectSuccessfulValueNotEmptyListSizeNotSameAsMaxAllowed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetKeyObjectSuccessfulValueNotEmptyJsonFailed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulListSizeisMaxAllowed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulListNotTruncated);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulGetMoreObjects);

  // Google test for object list response.
  FRIEND_TEST(S3ObjectListResponseTest, ObjectListResponseConstructorTest);
  FRIEND_TEST(S3ObjectListResponseTest, TestS3ObjectListResponseSetters);
  FRIEND_TEST(S3ObjectListResponseTest, TestS3ObjectListResponseGetters);
  FRIEND_TEST(S3ObjectListResponseTest,
              ObjectListResponseWithValidObjectsNotTruncated);
  FRIEND_TEST(S3ObjectListResponseTest,
              ObjectListResponseWithValidObjectsTruncated);
  FRIEND_TEST(S3ObjectListResponseTest,
              ObjectListMultiuploadResponseWithValidObjectNotTruncated);
  FRIEND_TEST(S3ObjectListResponseTest,
              ObjectListMultipartResponseWithValidObjectNotTruncated);
  FRIEND_TEST(S3ObjectListResponseTest,
              ObjectListMultipartResponseWithValidObjectTruncated);
};

#endif

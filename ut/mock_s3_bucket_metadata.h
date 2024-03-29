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

#ifndef __S3_UT_MOCK_S3_BUCKET_METADATA_H__
#define __S3_UT_MOCK_S3_BUCKET_METADATA_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "s3_bucket_metadata.h"

using ::testing::_;
using ::testing::Return;

class S3RequestObject;

class MockS3BucketMetadata : public S3BucketMetadata {
 public:
  explicit MockS3BucketMetadata(std::shared_ptr<S3RequestObject> s3_req_obj,
                                const std::string& bucket_name = "")
      : S3BucketMetadata(std::move(s3_req_obj), bucket_name) {}

  MOCK_METHOD2(load, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD(S3BucketMetadataState, get_state, (), (const));
  MOCK_METHOD1(set_state, void(S3BucketMetadataState state));
  MOCK_METHOD0(get_policy_as_json, std::string&());
  MOCK_METHOD0(get_acl_as_xml, std::string());
  MOCK_METHOD(void, setpolicy, (std::string), (override));
  MOCK_METHOD1(set_tags,
               void(const std::map<std::string, std::string>& tags_str));
  MOCK_METHOD1(setacl, void(const std::string& acl_str));
  MOCK_METHOD2(save, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(update, void(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed));
  MOCK_METHOD2(remove, void(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed));
  MOCK_METHOD0(deletepolicy, void());
  MOCK_METHOD0(delete_bucket_tags, void());
  MOCK_METHOD1(set_location_constraint, void(std::string location));
  MOCK_METHOD1(from_json, int(std::string content));
  MOCK_METHOD0(get_owner_canonical_id, const std::string&());
  MOCK_CONST_METHOD0(get_object_list_index_layout,
                     const struct s3_motr_idx_layout&());
  MOCK_CONST_METHOD0(get_objects_version_list_index_layout,
                     const struct s3_motr_idx_layout&());
  MOCK_CONST_METHOD0(get_extended_metadata_index_layout,
                     const struct s3_motr_idx_layout&());
};

#endif

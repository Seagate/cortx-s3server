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

#ifndef __S3_UT_MOCK_S3_OBJECT_METADATA_H__
#define __S3_UT_MOCK_S3_OBJECT_METADATA_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock_s3_motr_wrapper.h"
#include "s3_object_metadata.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3ObjectMetadata : public S3ObjectMetadata {
 public:
  MockS3ObjectMetadata(std::shared_ptr<S3RequestObject> req,
                       std::shared_ptr<MockS3Motr> motr_api = nullptr)
      : S3ObjectMetadata(req, false, "", nullptr, nullptr, motr_api) {}
  MOCK_METHOD0(get_state, S3ObjectMetadataState());
  MOCK_METHOD0(get_version_key_in_index, std::string());
  MOCK_METHOD0(get_oid, struct m0_uint128());
  MOCK_METHOD0(get_layout_id, int());
  MOCK_METHOD1(set_oid, void(struct m0_uint128));
  MOCK_METHOD1(set_md5, void(std::string));
  MOCK_METHOD0(reset_date_time_to_current, void());
  MOCK_METHOD0(get_md5, std::string());
  MOCK_METHOD0(get_object_name, std::string());
  MOCK_METHOD0(get_content_length, size_t());
  MOCK_METHOD1(set_content_length, void(std::string length));
  MOCK_METHOD0(get_content_length_str, std::string());
  MOCK_METHOD0(get_last_modified_gmt, std::string());
  MOCK_METHOD0(get_user_id, std::string());
  MOCK_METHOD0(get_user_name, std::string());
  MOCK_METHOD0(get_account_name, std::string());
  MOCK_METHOD0(get_canonical_id, std::string());
  MOCK_METHOD0(get_last_modified_iso, std::string());
  MOCK_METHOD0(get_storage_class, std::string());
  MOCK_METHOD0(get_upload_id, std::string());
  MOCK_METHOD0(check_object_tags_exists, bool());
  MOCK_METHOD0(delete_object_tags, void());
  MOCK_METHOD2(add_user_defined_attribute,
               void(std::string key, std::string val));
  MOCK_METHOD2(load, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(save, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(remove, void(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed));
  MOCK_METHOD1(setacl, void(const std::string&));
  MOCK_METHOD1(set_tags,
               void(const std::map<std::string, std::string>& tags_as_map));
  MOCK_METHOD2(save_metadata, void(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed));
  MOCK_METHOD1(from_json, int(std::string content));
};

#endif


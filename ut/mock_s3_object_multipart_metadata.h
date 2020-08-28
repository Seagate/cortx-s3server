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

#ifndef __S3_UT_MOCK_S3_OBJECT_MULTIPART_METADATA_H__
#define __S3_UT_MOCK_S3_OBJECT_MULTIPART_METADATA_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_request_object.h"
#include "s3_object_metadata.h"

using ::testing::_;
using ::testing::Return;

class MockS3ObjectMultipartMetadata : public S3ObjectMetadata {
 public:
  MockS3ObjectMultipartMetadata(std::shared_ptr<S3RequestObject> req,
                                std::shared_ptr<MockS3Motr> motr_api,
                                std::string upload_id)
      : S3ObjectMetadata(req, true, upload_id, nullptr, nullptr, motr_api) {}
  MOCK_METHOD0(get_state, S3ObjectMetadataState());
  MOCK_METHOD0(get_old_oid, struct m0_uint128());
  MOCK_METHOD0(get_oid, struct m0_uint128());
  MOCK_METHOD0(get_old_layout_id, int());
  MOCK_METHOD0(get_upload_id, std::string());
  MOCK_METHOD0(get_part_one_size, size_t());
  MOCK_METHOD1(set_part_one_size, void(size_t part_size));
  MOCK_METHOD0(get_layout_id, int());
  MOCK_METHOD2(load, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(save, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(remove, void(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed));
  MOCK_METHOD0(get_user_attributes, std::map<std::string, std::string>&());
};

#endif

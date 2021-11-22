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

#pragma once

#ifndef __S3_UT_MOCK_S3_OBJECT_EXTND_METADATA_H__
#define __S3_UT_MOCK_S3_OBJECT_EXTND_METADATA_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock_s3_motr_wrapper.h"
#include "s3_object_metadata.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3ObjectExtendedMetadata : public S3ObjectExtendedMetadata {
 public:
  MockS3ObjectExtendedMetadata(std::shared_ptr<S3RequestObject> req,
                               std::shared_ptr<MockS3Motr> motr_api = nullptr)
      : S3ObjectExtendedMetadata(req, "", "", "", 0, 0, nullptr, nullptr,
                                 motr_api) {}
  MOCK_METHOD1(get_obj_ext_entries, void(std::string));
  MOCK_METHOD2(from_json, int(std::string, std::string));
  MOCK_METHOD1(get_json_str, std::string(struct s3_part_frag_context&));
  MOCK_METHOD0(has_entries, bool());
  MOCK_METHOD0(
      get_raw_extended_entries,
      const std::map<int, std::vector<struct s3_part_frag_context>>&());
  MOCK_METHOD0(get_fragment_count, unsigned int());
  MOCK_METHOD0(get_part_count, unsigned int());
  MOCK_METHOD2(load,
               void(std::function<void(void)>, std::function<void(void)>));
  MOCK_METHOD2(save,
               void(std::function<void(void)>, std::function<void(void)>));
  MOCK_METHOD0(get_kv_list_of_extended_entries,
               std::map<std::string, std::string>());
  MOCK_METHOD3(add_extended_entry,
               void(struct s3_part_frag_context&, unsigned int, unsigned int));
  MOCK_METHOD2(remove,
               void(std::function<void(void)>, std::function<void(void)>));
};

#endif

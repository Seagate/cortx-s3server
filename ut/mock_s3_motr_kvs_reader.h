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

#ifndef __S3_UT_MOCK_S3_MOTR_KVS_READER_H__
#define __S3_UT_MOCK_S3_MOTR_KVS_READER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_motr_kvs_reader.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3MotrKVSReader : public S3MotrKVSReader {
 public:
  MockS3MotrKVSReader(std::shared_ptr<RequestObject> req,
                      std::shared_ptr<MotrAPI> s3motr_api)
      : S3MotrKVSReader(req, s3motr_api) {}
  MOCK_METHOD0(get_state, S3MotrKVSReaderOpState());
  MOCK_METHOD0(get_value, std::string());
  MOCK_METHOD0(get_key_values,
               std::map<std::string, std::pair<int, std::string>> &());
  MOCK_METHOD3(lookup_index,
               void(struct m0_uint128, std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD4(get_keyval,
               void(struct m0_uint128 oid, std::vector<std::string> keys,
                    std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD4(get_keyval, void(struct m0_uint128 oid, std::string key,
                                std::function<void(void)> on_success,
                                std::function<void(void)> on_failed));
  MOCK_METHOD6(next_keyval,
               void(struct m0_uint128 oid, std::string key, size_t nr_kvp,
                    std::function<void(void)> on_success,
                    std::function<void(void)> on_failed, unsigned int flag));
};

#endif

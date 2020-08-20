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

#ifndef __S3_UT_MOCK_S3_MOTR_READER_H__
#define __S3_UT_MOCK_S3_MOTR_READER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_motr_reader.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3MotrReader : public S3MotrReader {
 public:
  MockS3MotrReader(std::shared_ptr<RequestObject> req, struct m0_uint128 oid,
                   int layout_id, std::shared_ptr<MotrAPI> motr_api = nullptr)
      : S3MotrReader(req, oid, layout_id, motr_api) {}
  MOCK_METHOD0(get_state, S3MotrReaderOpState());
  MOCK_METHOD0(get_oid, struct m0_uint128());
  MOCK_METHOD0(get_value, std::string());
  MOCK_METHOD1(set_oid, void(struct m0_uint128 oid));
  MOCK_METHOD3(read_object_data,
               bool(size_t num_of_blocks, std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD1(get_first_block, size_t(char** data));
  MOCK_METHOD1(get_next_block, size_t(char** data));
};

#endif

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

#ifndef __S3_UT_MOCK_S3_MOTR_WRITER_H__
#define __S3_UT_MOCK_S3_MOTR_WRITER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_motr_wrapper.h"
#include "s3_motr_writer.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3MotrWiter : public S3MotrWiter {
 public:
  MockS3MotrWiter(std::shared_ptr<RequestObject> req, struct m0_uint128 oid,
                  std::shared_ptr<MockS3Motr> s3_motr_mock_ptr)
      : S3MotrWiter(req, oid, 0, s3_motr_mock_ptr) {}
  MockS3MotrWiter(std::shared_ptr<RequestObject> req,
                  std::shared_ptr<MockS3Motr> s3_motr_mock_ptr)
      : S3MotrWiter(req, 0, s3_motr_mock_ptr) {}

  MOCK_METHOD0(get_state, S3MotrWiterOpState());
  MOCK_METHOD0(get_oid, struct m0_uint128());
  MOCK_METHOD0(get_content_md5, std::string());
  MOCK_METHOD1(get_op_ret_code_for, int(int));
  MOCK_METHOD1(get_op_ret_code_for_delete_op, int(int));
  MOCK_METHOD3(create_object,
               void(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed, int layout_id));
  MOCK_METHOD3(delete_object,
               void(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed, int layout_id));
  MOCK_METHOD3(delete_index,
               void(struct m0_uint128 oid, std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD4(delete_objects, void(std::vector<struct m0_uint128> oids,
                                    std::vector<int> layoutids,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed));
  MOCK_METHOD1(set_oid, void(struct m0_uint128 oid));
  MOCK_METHOD3(write_content,
               void(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed,
                    std::shared_ptr<S3AsyncBufferOptContainer> buffer));
};

#endif

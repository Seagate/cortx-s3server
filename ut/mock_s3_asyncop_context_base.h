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

#ifndef __S3_UT_MOCK_S3_ASYNCOP_CONTEXT_BASE_H__
#define __S3_UT_MOCK_S3_ASYNCOP_CONTEXT_BASE_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock_s3_motr_wrapper.h"
#include "s3_asyncop_context_base.h"
#include "s3_common.h"

class MockS3AsyncOpContextBase : public S3AsyncOpContextBase {
 public:
  MockS3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req,
                           std::function<void(void)> success_callback,
                           std::function<void(void)> failed_callback,
                           std::shared_ptr<MotrAPI> motr_api = nullptr)
      // Pass default opcount value of 1 explicitly.
      : S3AsyncOpContextBase(req, success_callback, failed_callback, 1,
                             motr_api) {
    EXPECT_CALL(*this, get_errno_for(0))
        .WillRepeatedly(::testing::Return(-ENOENT));
  }
  MOCK_METHOD2(set_op_errno_for, void(int op_idx, int err));
  MOCK_METHOD3(set_op_status_for,
               void(int op_idx, S3AsyncOpStatus opstatus, std::string message));
  MOCK_METHOD1(get_errno_for, int(int op_idx));
};

#endif

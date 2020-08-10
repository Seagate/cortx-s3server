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

#ifndef __S3_UT_MOCK_S3_ASYNC_BUFFER_OPT_H__
#define __S3_UT_MOCK_S3_ASYNC_BUFFER_OPT_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_async_buffer_opt.h"

using ::testing::_;
using ::testing::Return;

class MockS3AsyncBufferOptContainer : public S3AsyncBufferOptContainer {
 public:
  MockS3AsyncBufferOptContainer(size_t size_of_each_buf)
      : S3AsyncBufferOptContainer(size_of_each_buf) {}
  MOCK_METHOD0(is_freezed, bool());
  MOCK_METHOD0(get_content_length, size_t());
};

#endif

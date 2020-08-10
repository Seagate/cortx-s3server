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

#ifndef __S3_UT_MOCK_S3_MEMORY_PROFILE_H__
#define __S3_UT_MOCK_S3_MEMORY_PROFILE_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_memory_profile.h"

class MockS3MemoryProfile : public S3MemoryProfile {
 public:
  MockS3MemoryProfile() : S3MemoryProfile() {}
  MOCK_METHOD1(we_have_enough_memory_for_put_obj, bool(int layout_id));
  MOCK_METHOD0(free_memory_in_pool_above_threshold_limits, bool());
};

#endif

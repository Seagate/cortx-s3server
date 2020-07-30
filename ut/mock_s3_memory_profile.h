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

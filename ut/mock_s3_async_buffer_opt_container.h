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

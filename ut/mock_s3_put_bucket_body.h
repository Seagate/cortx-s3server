#pragma once

#ifndef __S3_UT_MOCK_S3_PUT_BUCKET_BODY_H__
#define __S3_UT_MOCK_S3_PUT_BUCKET_BODY_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_put_bucket_body.h"

using ::testing::_;
using ::testing::Return;

class MockS3PutBucketBody : public S3PutBucketBody {
 public:
  MockS3PutBucketBody(std::string& xml) : S3PutBucketBody(xml) {}
  MOCK_METHOD0(isOK, bool());
  MOCK_METHOD0(get_location_constraint, std::string());
};

#endif

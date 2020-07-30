#pragma once

#ifndef __S3_UT_MOCK_S3_API_HANDLER_H__
#define __S3_UT_MOCK_S3_API_HANDLER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_api_handler.h"

class MockS3ServiceAPIHandler : public S3ServiceAPIHandler {
 public:
  MockS3ServiceAPIHandler(std::shared_ptr<S3RequestObject> req,
                          S3OperationCode op_code)
      : S3ServiceAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockS3BucketAPIHandler : public S3BucketAPIHandler {
 public:
  MockS3BucketAPIHandler(std::shared_ptr<S3RequestObject> req,
                         S3OperationCode op_code)
      : S3BucketAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockS3ObjectAPIHandler : public S3ObjectAPIHandler {
 public:
  MockS3ObjectAPIHandler(std::shared_ptr<S3RequestObject> req,
                         S3OperationCode op_code)
      : S3ObjectAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockS3APIHandlerFactory : public S3APIHandlerFactory {
 public:
  MOCK_METHOD3(create_api_handler,
               std::shared_ptr<S3APIHandler>(
                   S3ApiType api_type, std::shared_ptr<S3RequestObject> request,
                   S3OperationCode op_code));
};

#endif

#pragma once

#ifndef __S3_UT_MOCK_S3_URI_H__
#define __S3_UT_MOCK_S3_URI_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_uri.h"

class MockS3PathStyleURI : public S3PathStyleURI {
 public:
  MockS3PathStyleURI(std::shared_ptr<S3RequestObject> req)
      : S3PathStyleURI(req) {}
  MOCK_METHOD0(get_bucket_name, std::string&());
  MOCK_METHOD0(get_object_name, std::string&());
  MOCK_METHOD0(get_operation_code, S3OperationCode());
  MOCK_METHOD0(get_s3_api_type, S3ApiType());
};

class MockS3VirtualHostStyleURI : public S3VirtualHostStyleURI {
 public:
  MockS3VirtualHostStyleURI(std::shared_ptr<S3RequestObject> req)
      : S3VirtualHostStyleURI(req) {}
  MOCK_METHOD0(get_bucket_name, std::string&());
  MOCK_METHOD0(get_object_name, std::string&());
  MOCK_METHOD0(get_operation_code, S3OperationCode());
  MOCK_METHOD0(get_s3_api_type, S3ApiType());
};

class MockS3UriFactory : public S3UriFactory {
 public:
  MOCK_METHOD2(create_uri_object,
               S3URI*(S3UriType uri_type,
                      std::shared_ptr<S3RequestObject> request));
};
#endif

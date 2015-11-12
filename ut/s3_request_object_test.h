#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_REQUEST_OBJECT_TEST_H__
#define __MERO_FE_S3_SERVER_S3_REQUEST_OBJECT_TEST_H__

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "s3_request_object.h"

class MockS3RequestObject : public S3RequestObject {
  public:
  MockS3RequestObject(evhtp_request_t *req):S3RequestObject(req) {}
  MOCK_METHOD0(c_get_full_path, const char *());
  MOCK_METHOD0(get_host_header, std::string());
  MOCK_METHOD1(has_query_param_key, bool(std::string key));
};

#endif

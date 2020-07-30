#pragma once

#ifndef __S3_UT_MOCK_S3_AUTH_CLIENT_H__
#define __S3_UT_MOCK_S3_AUTH_CLIENT_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_auth_client.h"

class MockS3AuthClientOpContext : public S3AuthClientOpContext {
 public:
  MockS3AuthClientOpContext(std::shared_ptr<S3RequestObject> req,
                            std::function<void()> success_callback,
                            std::function<void()> failed_callback)
      : S3AuthClientOpContext(req, success_callback, failed_callback) {}
  MOCK_METHOD0(init_auth_op_ctx, bool());
  MOCK_METHOD1(create_basic_auth_op_ctx,
               struct s3_auth_op_context *(struct event_base *eventbase));
};

class MockS3AuthClient : public S3AuthClient {
 public:
  MockS3AuthClient(std::shared_ptr<S3RequestObject> req) : S3AuthClient(req) {}
  MOCK_METHOD1(execute_authconnect_request,
               void(struct s3_auth_op_context *auth_ctx));
  MOCK_METHOD2(init_chunk_auth_cycle,
               void(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD2(add_checksum_for_chunk,
               void(std::string current_sign, std::string sha256_of_payload));
  MOCK_METHOD2(add_last_checksum_for_chunk,
               void(std::string current_sign, std::string sha256_of_payload));
};
#endif

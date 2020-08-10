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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "s3_api_handler.h"

#include "mock_s3_async_buffer_opt_container.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

class S3APIHandlerFactoryTest : public testing::Test {
 protected:
  S3APIHandlerFactoryTest() {
    S3Option::get_instance()->disable_auth();

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";
    object_name = "objname";

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr,
                                                         async_buffer_factory);
    EXPECT_CALL(*mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));

    EXPECT_CALL(*mock_request, get_query_string_value("prefix"))
        .WillRepeatedly(Return(""));
    EXPECT_CALL(*mock_request, get_query_string_value("delimiter"))
        .WillRepeatedly(Return("/"));
    EXPECT_CALL(*mock_request, get_query_string_value("marker"))
        .WillRepeatedly(Return(""));
    EXPECT_CALL(*mock_request, get_query_string_value("max-keys"))
        .WillRepeatedly(Return("1000"));
    EXPECT_CALL(*mock_request, get_query_string_value("encoding-type"))
        .WillRepeatedly(Return(""));

    factory_under_test.reset(new S3APIHandlerFactory());

    EXPECT_CALL(*(mock_request), http_verb())
        .WillRepeatedly(Return(S3HttpVerb::GET));
  }

  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;

  std::shared_ptr<S3APIHandlerFactory> factory_under_test;
  std::string bucket_name, object_name;
};

TEST_F(S3APIHandlerFactoryTest, ShouldCreateS3ServiceAPIHandler) {
  std::map<std::string, std::string> input_headers;
  input_headers["Authorization"] = "1";
  EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));
  std::shared_ptr<S3APIHandler> handler =
      factory_under_test->create_api_handler(S3ApiType::service, mock_request,
                                             S3OperationCode::none);

  EXPECT_FALSE((dynamic_cast<S3ServiceAPIHandler *>(handler.get())) == nullptr);
}

TEST_F(S3APIHandlerFactoryTest, ShouldCreateS3BucketAPIHandler) {
  std::map<std::string, std::string> input_headers;
  input_headers["Authorization"] = "1";
  EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));
  std::shared_ptr<S3APIHandler> handler =
      factory_under_test->create_api_handler(S3ApiType::bucket, mock_request,
                                             S3OperationCode::none);

  EXPECT_FALSE((dynamic_cast<S3BucketAPIHandler *>(handler.get())) == nullptr);
}

TEST_F(S3APIHandlerFactoryTest, ShouldCreateS3ObjectAPIHandler) {
  std::map<std::string, std::string> input_headers;
  input_headers["Authorization"] = "1";
  EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));
  std::shared_ptr<S3APIHandler> handler =
      factory_under_test->create_api_handler(S3ApiType::object, mock_request,
                                             S3OperationCode::none);

  EXPECT_FALSE((dynamic_cast<S3ObjectAPIHandler *>(handler.get())) == nullptr);
}

TEST_F(S3APIHandlerFactoryTest, ShouldCreateS3FaultinjectionAPIHandler) {
  std::shared_ptr<S3APIHandler> handler =
      factory_under_test->create_api_handler(
          S3ApiType::faultinjection, mock_request, S3OperationCode::none);

  EXPECT_FALSE((dynamic_cast<S3FaultinjectionAPIHandler *>(handler.get())) ==
               nullptr);
}


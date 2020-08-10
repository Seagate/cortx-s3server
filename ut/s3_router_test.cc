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

#include "gtest/gtest.h"

#include "s3_error_codes.h"
#include "s3_router.h"

#include "mock_s3_api_handler.h"
#include "mock_s3_request_object.h"
#include "mock_s3_uri.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::Return;
using ::testing::Mock;
using ::testing::ReturnRef;

// To use a test fixture, derive a class from testing::Test.
class S3RouterTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3RouterTest() { router = new S3Router(NULL, NULL); }

  ~S3RouterTest() { delete router; }

  // Declares the variables your tests want to use.
  S3Router *router;
};

class S3RouterDispatchTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3RouterDispatchTest() {
    mock_api_handler_factory = new MockS3APIHandlerFactory();
    uri_factory = new S3UriFactory();
    router = new S3Router(mock_api_handler_factory, uri_factory);

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
  }

  ~S3RouterDispatchTest() { delete router; }

  // Declares the variables your tests want to use.
  S3Router *router;

  MockS3APIHandlerFactory *mock_api_handler_factory;
  S3UriFactory *uri_factory;
  std::shared_ptr<MockS3RequestObject> mock_request;
};

TEST_F(S3RouterTest, ReturnsTrueForMatchingDefaultEP) {
  std::string valid_ep = "s3.seagate.com";
  EXPECT_TRUE(router->is_default_endpoint(valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForMisMatchOfDefaultEP) {
  std::string diff_valid_ep = "someregion.s3.seagate.com";
  EXPECT_FALSE(router->is_default_endpoint(diff_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForEmptyDefaultEP) {
  std::string diff_in_valid_ep = "";
  EXPECT_FALSE(router->is_default_endpoint(diff_in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingEP) {
  std::string valid_ep = "s3.seagate.com";
  EXPECT_TRUE(router->is_exact_valid_endpoint(valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingRegionEP) {
  std::string valid_ep = "s3-us.seagate.com";
  EXPECT_TRUE(router->is_exact_valid_endpoint(valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForMisMatchRegionEP) {
  std::string in_valid_ep = "s3-invalid.seagate.com";
  EXPECT_FALSE(router->is_exact_valid_endpoint(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForEmptyRegionEP) {
  std::string in_valid_ep = "";
  EXPECT_FALSE(router->is_exact_valid_endpoint(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingSubEP) {
  std::string valid_ep = "ABC.s3.seagate.com";
  EXPECT_TRUE(router->is_subdomain_match(valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingSubRegionEP) {
  std::string valid_ep = "XYZ.s3-us.seagate.com";
  EXPECT_TRUE(router->is_subdomain_match(valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingEUSubRegionEP) {
  std::string valid_ep = "XYZ.s3-europe.seagate.com";
  EXPECT_TRUE(router->is_subdomain_match(valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForMisMatchSubRegionEP) {
  std::string in_valid_ep = "ABC.s3-invalid.seagate.com";
  EXPECT_FALSE(router->is_subdomain_match(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForInvalidEP) {
  std::string in_valid_ep = "ABC.s3-invalid.google.com";
  EXPECT_FALSE(router->is_subdomain_match(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForEmptyEP) {
  std::string in_valid_ep = "";
  EXPECT_FALSE(router->is_subdomain_match(in_valid_ep));
}

TEST_F(S3RouterDispatchTest, InvokesServiceApi) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, get_header_value("x-seagate-faultinjection"))
      .Times(1)
      .WillRepeatedly(Return(""));

  EXPECT_CALL(*mock_request, c_get_full_path()).WillRepeatedly(Return("/"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);

  std::shared_ptr<MockS3ServiceAPIHandler> mock_api_handler =
      std::make_shared<MockS3ServiceAPIHandler>(mock_request,
                                                S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::service), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesBucketApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, has_query_param_key(_))
      .WillRepeatedly(Return(false));

  std::shared_ptr<MockS3BucketAPIHandler> mock_api_handler =
      std::make_shared<MockS3BucketAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::bucket), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesBucketLocationApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;
  query_params["location"] = "";

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);

  std::shared_ptr<MockS3BucketAPIHandler> mock_api_handler =
      std::make_shared<MockS3BucketAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::bucket), Eq(mock_request),
                                 Eq(S3OperationCode::location)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesBucketAclApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;
  query_params["acl"] = "";

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);

  std::shared_ptr<MockS3BucketAPIHandler> mock_api_handler =
      std::make_shared<MockS3BucketAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::bucket), Eq(mock_request),
                                 Eq(S3OperationCode::acl)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesObjectApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket/readme.md"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq("readme.md"))).Times(1);

  std::shared_ptr<MockS3ObjectAPIHandler> mock_api_handler =
      std::make_shared<MockS3ObjectAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::object), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesObjectApiWithPathStyleObjNameDirStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket/internal/readme.md"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq("internal/readme.md")))
      .Times(1);

  std::shared_ptr<MockS3ObjectAPIHandler> mock_api_handler =
      std::make_shared<MockS3ObjectAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::object), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesObjectApiWithPathStyleEmptyHostHeader) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket/readme.md"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq("readme.md"))).Times(1);

  std::shared_ptr<MockS3ObjectAPIHandler> mock_api_handler =
      std::make_shared<MockS3ObjectAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::object), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest,
       InvokesObjectApiWithPathStyleNonDefaultHostHeader) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("server"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/seagate_bucket/readme.md"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq("readme.md"))).Times(1);

  std::shared_ptr<MockS3ObjectAPIHandler> mock_api_handler =
      std::make_shared<MockS3ObjectAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::object), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

// Virtual host style tests
TEST_F(S3RouterDispatchTest, InvokesBucketApiWithVirtualHostStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("seagate_bucket.s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path()).WillRepeatedly(Return("/"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);

  std::shared_ptr<MockS3BucketAPIHandler> mock_api_handler =
      std::make_shared<MockS3BucketAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::bucket), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesBucketLocationApiWithVirtualHostStyle) {
  std::map<std::string, std::string, compare> query_params;
  query_params["location"] = "";

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("seagate_bucket.s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path()).WillRepeatedly(Return("/"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);

  std::shared_ptr<MockS3BucketAPIHandler> mock_api_handler =
      std::make_shared<MockS3BucketAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::bucket), Eq(mock_request),
                                 Eq(S3OperationCode::location)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesBucketAclApiWithVirtualHostStyle) {
  std::map<std::string, std::string, compare> query_params;
  query_params["acl"] = "";

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("seagate_bucket.s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path()).WillRepeatedly(Return("/"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq(""))).Times(1);

  std::shared_ptr<MockS3BucketAPIHandler> mock_api_handler =
      std::make_shared<MockS3BucketAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::bucket), Eq(mock_request),
                                 Eq(S3OperationCode::acl)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest, InvokesObjectApiWithVirtualHostStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("seagate_bucket.s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/readme.md"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq("readme.md"))).Times(1);

  std::shared_ptr<MockS3ObjectAPIHandler> mock_api_handler =
      std::make_shared<MockS3ObjectAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::object), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(S3RouterDispatchTest,
       InvokesObjectApiWithVirtualHostStyleObjNameDirStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("seagate_bucket.s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillRepeatedly(Return("/internal/readme.md"));
  EXPECT_CALL(*mock_request, set_bucket_name(StrEq("seagate_bucket"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_name(StrEq("internal/readme.md")))
      .Times(1);

  std::shared_ptr<MockS3ObjectAPIHandler> mock_api_handler =
      std::make_shared<MockS3ObjectAPIHandler>(mock_request,
                                               S3OperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(S3ApiType::object), Eq(mock_request),
                                 Eq(S3OperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

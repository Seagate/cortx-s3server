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

#include "mock_mero_api_handler.h"
#include "mock_mero_request_object.h"
#include "mock_mero_uri.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::Return;
using ::testing::Mock;
using ::testing::ReturnRef;

class MeroRouterDispatchTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  MeroRouterDispatchTest() {
    mock_api_handler_factory = new MockMeroAPIHandlerFactory();
    uri_factory = new MeroUriFactory();
    router = new MeroRouter(mock_api_handler_factory, uri_factory);

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    mock_request = std::make_shared<MockMeroRequestObject>(req, evhtp_obj_ptr);
  }

  ~MeroRouterDispatchTest() { delete router; }

  // Declares the variables your tests want to use.
  MeroRouter *router;

  MockMeroAPIHandlerFactory *mock_api_handler_factory;
  MeroUriFactory *uri_factory;
  std::shared_ptr<MockMeroRequestObject> mock_request;
};

TEST_F(MeroRouterDispatchTest, InvokesKeyValueApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/indexes/123-456/Object-key1"));
  EXPECT_CALL(*mock_request, set_key_name(StrEq("Object-key1"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_oid_lo(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_object_oid_hi(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_index_id_lo(StrEq("456"))).Times(1);
  EXPECT_CALL(*mock_request, set_index_id_hi(StrEq("123"))).Times(1);
  EXPECT_CALL(*mock_request, has_query_param_key(_))
      .WillRepeatedly(Return(false));

  std::shared_ptr<MockMeroKeyValueAPIHandler> mock_api_handler =
      std::make_shared<MockMeroKeyValueAPIHandler>(mock_request,
                                                   MeroOperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(MeroApiType::keyval), Eq(mock_request),
                                 Eq(MeroOperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(MeroRouterDispatchTest, InvokesIndexApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;
  query_params["location"] = "";

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/indexes/123-456"));
  EXPECT_CALL(*mock_request, set_key_name(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_object_oid_lo(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_object_oid_hi(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_index_id_lo(StrEq("456"))).Times(1);
  EXPECT_CALL(*mock_request, set_index_id_hi(StrEq("123"))).Times(1);

  std::shared_ptr<MockMeroIndexAPIHandler> mock_api_handler =
      std::make_shared<MockMeroIndexAPIHandler>(mock_request,
                                                MeroOperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(MeroApiType::index), Eq(mock_request),
                                 Eq(MeroOperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(MeroRouterDispatchTest, InvokesObjectOidApiWithPathStyle) {
  std::map<std::string, std::string, compare> query_params;
  query_params["acl"] = "";

  EXPECT_CALL(*mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*mock_request, get_host_header())
      .WillRepeatedly(Return("s3.seagate.com"));
  EXPECT_CALL(*mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/objects/123-456"));
  EXPECT_CALL(*mock_request, set_key_name(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_object_oid_lo(StrEq("456"))).Times(1);
  EXPECT_CALL(*mock_request, set_object_oid_hi(StrEq("123"))).Times(1);
  EXPECT_CALL(*mock_request, set_index_id_lo(StrEq(""))).Times(1);
  EXPECT_CALL(*mock_request, set_index_id_hi(StrEq(""))).Times(1);

  std::shared_ptr<MockMeroObjectAPIHandler> mock_api_handler =
      std::make_shared<MockMeroObjectAPIHandler>(mock_request,
                                                 MeroOperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(MeroApiType::object), Eq(mock_request),
                                 Eq(MeroOperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

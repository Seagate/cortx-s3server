/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 9-JULY-2019
 */

#include "gtest/gtest.h"

#include "s3_error_codes.h"
#include "s3_router.h"

#include "mock_motr_api_handler.h"
#include "mock_motr_request_object.h"
#include "mock_motr_uri.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::Return;
using ::testing::Mock;
using ::testing::ReturnRef;

class MotrRouterDispatchTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  MotrRouterDispatchTest() {
    mock_api_handler_factory = new MockMotrAPIHandlerFactory();
    uri_factory = new MotrUriFactory();
    router = new MotrRouter(mock_api_handler_factory, uri_factory);

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    mock_request = std::make_shared<MockMotrRequestObject>(req, evhtp_obj_ptr);
  }

  ~MotrRouterDispatchTest() { delete router; }

  // Declares the variables your tests want to use.
  MotrRouter *router;

  MockMotrAPIHandlerFactory *mock_api_handler_factory;
  MotrUriFactory *uri_factory;
  std::shared_ptr<MockMotrRequestObject> mock_request;
};

TEST_F(MotrRouterDispatchTest, InvokesKeyValueApiWithPathStyle) {
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

  std::shared_ptr<MockMotrKeyValueAPIHandler> mock_api_handler =
      std::make_shared<MockMotrKeyValueAPIHandler>(mock_request,
                                                   MotrOperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(MotrApiType::keyval), Eq(mock_request),
                                 Eq(MotrOperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(MotrRouterDispatchTest, InvokesIndexApiWithPathStyle) {
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

  std::shared_ptr<MockMotrIndexAPIHandler> mock_api_handler =
      std::make_shared<MockMotrIndexAPIHandler>(mock_request,
                                                MotrOperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(MotrApiType::index), Eq(mock_request),
                                 Eq(MotrOperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}

TEST_F(MotrRouterDispatchTest, InvokesObjectOidApiWithPathStyle) {
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

  std::shared_ptr<MockMotrObjectAPIHandler> mock_api_handler =
      std::make_shared<MockMotrObjectAPIHandler>(mock_request,
                                                 MotrOperationCode::none);

  EXPECT_CALL(*mock_api_handler_factory,
              create_api_handler(Eq(MotrApiType::object), Eq(mock_request),
                                 Eq(MotrOperationCode::none)))
      .WillOnce(Return(mock_api_handler));

  EXPECT_CALL(*mock_api_handler, dispatch()).Times(1);

  router->dispatch(mock_request);

  // ensure we release the api handlers internal reference.
  mock_api_handler->i_am_done();
}
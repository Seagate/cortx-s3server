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

#include "motr_api_handler.h"

#include "mock_s3_async_buffer_opt_container.h"
#include "mock_s3_factory.h"
#include "mock_motr_request_object.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::ReturnRef;

class MotrAPIHandlerTest : public testing::Test {
 protected:
  MotrAPIHandlerTest() {
    S3Option::get_instance()->disable_auth();

    call_count_one = 0;

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    mock_request = std::make_shared<MockMotrRequestObject>(
        req, evhtp_obj_ptr, async_buffer_factory);
  }

  std::shared_ptr<MockMotrRequestObject> mock_request;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;

  std::shared_ptr<MotrAPIHandler> handler_under_test;

  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(MotrAPIHandlerTest, ConstructorTest) {
  // Creation handler per test as it will be specific
  handler_under_test.reset(
      new MotrIndexAPIHandler(mock_request, MotrOperationCode::none));

  EXPECT_EQ(MotrOperationCode::none, handler_under_test->operation_code);
}

TEST_F(MotrAPIHandlerTest, ManageSelfAndReset) {
  // Creation handler per test as it will be specific
  handler_under_test.reset(
      new MotrIndexAPIHandler(mock_request, MotrOperationCode::none));
  handler_under_test->manage_self(handler_under_test);
  EXPECT_TRUE(handler_under_test == handler_under_test->_get_self_ref());
  handler_under_test->i_am_done();
  EXPECT_TRUE(handler_under_test->_get_self_ref() == nullptr);
}

TEST_F(MotrAPIHandlerTest, DispatchActionTest) {
  // Creation handler per test as it will be specific
  std::map<std::string, std::string> input_headers;
  input_headers["Authorization"] = "1";
  EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));
  handler_under_test.reset(
      new MotrIndexAPIHandler(mock_request, MotrOperationCode::none));

  EXPECT_CALL(*(mock_request), http_verb()).WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*(mock_request), get_api_type())
      .WillRepeatedly(Return(MotrApiType::index));

  handler_under_test->create_action();

  //
  handler_under_test->_get_action()->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(handler_under_test->_get_action(),
                         MotrAPIHandlerTest::func_callback_one, this);

  handler_under_test->dispatch();

  EXPECT_EQ(1, call_count_one);
  handler_under_test->_get_action()->i_am_done();
}

TEST_F(MotrAPIHandlerTest, DispatchUnSupportedAction) {
  // Creation handler per test as it will be specific
  handler_under_test.reset(
      new MotrIndexAPIHandler(mock_request, MotrOperationCode::none));

  EXPECT_CALL(*(mock_request), set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*(mock_request), send_response(501, _)).Times(1);

  handler_under_test->dispatch();
}

TEST_F(MotrAPIHandlerTest, DispatchUnSupportedAction1) {

  handler_under_test.reset(
      new MotrObjectAPIHandler(mock_request, MotrOperationCode::none));

  EXPECT_CALL(*(mock_request), set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*(mock_request), send_response(501, _)).Times(1);
  handler_under_test->dispatch();
}

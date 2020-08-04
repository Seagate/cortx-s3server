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

#include <memory>

#include "mock_s3_factory.h"
#include "s3_error_codes.h"
#include "s3_head_service_action.h"
#include "s3_test_utils.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrEq;

class S3HeadServiceActionTest : public testing::Test {
 protected:
  S3HeadServiceActionTest() {

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    // Mock objects.
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);

    // Object to be tested.
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test.reset(new S3HeadServiceAction(ptr_mock_request));
  }

  std::shared_ptr<S3HeadServiceAction> action_under_test;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
};

// Test that checks if fields of S3HeadServiceActionTest object have been
// initialized to their default value.
TEST_F(S3HeadServiceActionTest, ConstructorTest) {
  // Number of tasks in task list should be only one.
  EXPECT_EQ(2, action_under_test->number_of_tasks());
}

TEST_F(S3HeadServiceActionTest, SendResponseToClientServiceUnavailable) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(AtLeast(1));

  action_under_test->check_shutdown_and_rollback();
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3HeadServiceActionTest, SendResponseToClientSuccess) {
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

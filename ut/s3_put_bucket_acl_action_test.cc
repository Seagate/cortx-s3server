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

#include "s3_put_bucket_acl_action.h"
#include "gtest/gtest.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_error_codes.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;
using ::testing::AtLeast;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_bucket_info();                               \
  } while (0)

class S3PutBucketAclActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3PutBucketAclActionTest() {
    call_count_one = 0;
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";
    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(mock_request);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test.reset(
        new S3PutBucketACLAction(mock_request, bucket_meta_factory));
  }

  std::shared_ptr<S3PutBucketACLAction> action_under_test;
  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  int call_count_one;
  std::string bucket_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PutBucketAclActionTest, Constructor) {
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PutBucketAclActionTest, ValidateOnAllDataShouldCallNext) {
  std::string acl = "<AccessControlPolicy>...</AccessControlPolicy>";

  EXPECT_CALL(*mock_request, has_all_body_content()).WillOnce(Return(true));
  EXPECT_CALL(*mock_request, get_full_body_content_as_string())
      .WillOnce(ReturnRef(acl));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutBucketAclActionTest::func_callback_one, this);

  action_under_test->validate_request();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutBucketAclActionTest, ValidateOnPartialDataShouldWaitForMore) {
  std::string acl = "<AccessControlPolicy>...</AccessControlPolicy>";

  EXPECT_CALL(*mock_request, has_all_body_content()).WillOnce(Return(false));
  EXPECT_CALL(*mock_request, listen_for_incoming_data(_, _)).Times(1);
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutBucketAclActionTest::func_callback_one, this);

  action_under_test->validate_request();
}

TEST_F(S3PutBucketAclActionTest, FetchBucketInfo) {
  CREATE_BUCKET_METADATA;
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PutBucketAclActionTest, FetchBucketInfoFailedWithMissing) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed404, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PutBucketAclActionTest, FetchBucketInfoFailed) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PutBucketAclActionTest, SetAclShouldUpdateMetadata) {
  CREATE_BUCKET_METADATA;

  bucket_meta_factory->mock_bucket_metadata->set_state(
      S3BucketMetadataState::present);

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), setacl(_)).Times(1);
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), update(_, _))
      .Times(1);

  action_under_test->setacl();
}

TEST_F(S3PutBucketAclActionTest, SendResponseWhenShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, set_out_header_value(Eq("Retry-After"), Eq("1")))
      .Times(1);
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed503, _))
      .Times(AtLeast(1));

  // send_response_to_s3_client is called in check_shutdown_and_rollback
  action_under_test->check_shutdown_and_rollback();

  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutBucketAclActionTest, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutBucketAclActionTest, SendAnyFailedResponse) {
  action_under_test->set_s3_error("NoSuchBucket");

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed404, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}


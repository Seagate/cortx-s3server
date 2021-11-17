/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_put_bucket_versioning_action.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::ReturnRef;

class S3PutBucketVersioningActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3PutBucketVersioningActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";

    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*request_mock, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));

    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(request_mock);
    bucket_versioning_body_factory_mock =
        std::make_shared<MockS3PutVersioningBodyFactory>(
            mock_bucket_version_str, mock_request_id);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*request_mock, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test_ptr = std::make_shared<S3PutBucketVersioningAction>(
        request_mock, bucket_meta_factory, bucket_versioning_body_factory_mock);
    mock_request_id = "mock_request_id";
    mock_bucket_version_str = "MockBucketVersion";

    call_count_one = 0;
  }

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<S3PutBucketVersioningAction> action_under_test_ptr;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3PutVersioningBodyFactory>
      bucket_versioning_body_factory_mock;
  std::string mock_bucket_versioning_state;
  std::string mock_bucket_version_str;
  std::string mock_request_id;
  int call_count_one;
  std::string bucket_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PutBucketVersioningActionTest, Constructor) {
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
  EXPECT_TRUE(action_under_test_ptr->put_bucket_version_factory != nullptr);
}

TEST_F(S3PutBucketVersioningActionTest, ValidateRequest) {

  mock_bucket_version_str =
      "<VersioningConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Status>Enabled</Status>"
      "</VersioningConfiguration>";
  call_count_one = 0;

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(mock_bucket_version_str));
  EXPECT_CALL(
      *(bucket_versioning_body_factory_mock->mock_put_bucket_versioning_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(
      *(bucket_versioning_body_factory_mock->mock_put_bucket_versioning_body),
      get_versioning_status())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(mock_bucket_versioning_state));

  action_under_test_ptr->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test_ptr,
                         S3PutBucketVersioningActionTest::func_callback_one,
                         this);
  action_under_test_ptr->validate_request();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutBucketVersioningActionTest, ValidateRequestMoreContent) {
  EXPECT_CALL(*request_mock, has_all_body_content()).Times(1).WillOnce(
      Return(false));
  EXPECT_CALL(*request_mock, get_data_length()).Times(1).WillOnce(Return(0));
  EXPECT_CALL(*request_mock, listen_for_incoming_data(_, _)).Times(1);

  action_under_test_ptr->validate_request();
}

TEST_F(S3PutBucketVersioningActionTest, ValidateInvalidRequest) {

  mock_bucket_version_str =
      "<VersioningConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Status>Enabled</Status>"
      "</VersioningConfiguration>";

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(mock_bucket_version_str));
  EXPECT_CALL(
      *(bucket_versioning_body_factory_mock->mock_put_bucket_versioning_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutBucketVersioningActionTest,
       ValidateRequestXmlVersioningConfiguration) {

  mock_bucket_version_str =
      "<VersioningConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Status>Enabled</Status>"
      "</VersioningConfiguration>";
  call_count_one = 0;

  action_under_test_ptr->put_bucket_version_body =
      bucket_versioning_body_factory_mock->create_put_bucket_versioning_body(
          mock_bucket_version_str, mock_request_id);

  EXPECT_CALL(
      *(bucket_versioning_body_factory_mock->mock_put_bucket_versioning_body),
      validate_bucket_xml_versioning_status(_))
      .Times(1)
      .WillOnce(Return(true));

  action_under_test_ptr->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test_ptr,
                         S3PutBucketVersioningActionTest::func_callback_one,
                         this);
  action_under_test_ptr->validate_request_xml_versioning_status();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutBucketVersioningActionTest, ValidateRequestXmlWithSuspendedState) {

  mock_bucket_version_str =
      "<VersioningConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Status>Suspended</Status>"
      "</VersioningConfiguration>";
  call_count_one = 0;

  action_under_test_ptr->put_bucket_version_body =
      bucket_versioning_body_factory_mock->create_put_bucket_versioning_body(
          mock_bucket_version_str, mock_request_id);

  EXPECT_CALL(
      *(bucket_versioning_body_factory_mock->mock_put_bucket_versioning_body),
      validate_bucket_xml_versioning_status(_))
      .Times(1)
      .WillOnce(Return(true));

  action_under_test_ptr->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test_ptr,
                         S3PutBucketVersioningActionTest::func_callback_one,
                         this);
  action_under_test_ptr->validate_request_xml_versioning_status();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutBucketVersioningActionTest, ValidateInvalidRequestXmlTags) {

  mock_bucket_version_str =
      "<VersioningConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Status>Unversioned</Status>"
      "</VersioningConfiguration>";

  action_under_test_ptr->put_bucket_version_body =
      bucket_versioning_body_factory_mock->create_put_bucket_versioning_body(
          mock_bucket_version_str, mock_request_id);

  EXPECT_CALL(
      *(bucket_versioning_body_factory_mock->mock_put_bucket_versioning_body),
      validate_bucket_xml_versioning_status(_))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->validate_request_xml_versioning_status();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutBucketVersioningActionTest, SetVersioningState) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillOnce(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              set_bucket_versioning(_)).Times(AtLeast(1));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), update(_, _))
      .Times(AtLeast(1));
  action_under_test_ptr->save_versioning_status_to_bucket_metadata();
}

TEST_F(S3PutBucketVersioningActionTest, SetVersioningStateWhenBucketMissing) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("NoSuchBucket",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutBucketVersioningActionTest, SetVersioningStateWhenBucketFailed) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutBucketVersioningActionTest,
       SetVersioningStateWhenBucketFailedToLaunch) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(503, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutBucketVersioningActionTest,
       SendResponseToClientServiceUnavailable) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*request_mock, pause()).Times(1);
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(503, _)).Times(AtLeast(1));
  action_under_test_ptr->check_shutdown_and_rollback();
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutBucketVersioningActionTest, SendResponseToClientMalformedXML) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  action_under_test_ptr->set_s3_error("MalformedXML");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutBucketVersioningActionTest, SendResponseToClientNoSuchBucket) {
  action_under_test_ptr->set_s3_error("NoSuchBucket");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutBucketVersioningActionTest, SendResponseToClientSuccess) {
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutBucketVersioningActionTest, SendResponseToClientInternalError) {
  action_under_test_ptr->set_s3_error("InternalError");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

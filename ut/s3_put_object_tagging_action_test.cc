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

#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_put_object_tagging_action.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::ReturnRef;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test_ptr->fetch_bucket_info();                           \
  } while (0)

#define CREATE_OBJECT_METADATA                                                 \
  do {                                                                         \
    action_under_test_ptr->object_metadata =                                   \
        object_meta_factory->create_object_metadata_obj(request_mock,          \
                                                        object_list_indx_oid); \
  } while (0)

class S3PutObjectTaggingActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3PutObjectTaggingActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";
    object_name = "objname";
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*request_mock, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*request_mock, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));

    object_list_indx_oid = {0x11ffff, 0x1ffff};
    object_meta_factory =
        std::make_shared<MockS3ObjectMetadataFactory>(request_mock);
    object_meta_factory->set_object_list_index_oid(object_list_indx_oid);

    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(request_mock);
    bucket_tag_body_factory_mock = std::make_shared<MockS3PutTagBodyFactory>(
        MockObjectTagsStr, MockRequestId);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*request_mock, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test_ptr = std::make_shared<S3PutObjectTaggingAction>(
        request_mock, bucket_meta_factory, object_meta_factory,
        bucket_tag_body_factory_mock);
    MockRequestId.assign("MockRequestId");
    MockObjectTagsStr.assign("MockObjectTags");
  }

  struct m0_uint128 object_list_indx_oid;
  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<S3PutObjectTaggingAction> action_under_test_ptr;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3PutTagBodyFactory> bucket_tag_body_factory_mock;
  std::map<std::string, std::string> MockObjectTags;
  std::string MockObjectTagsStr;
  std::string MockRequestId;
  int call_count_one;
  std::string bucket_name, object_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PutObjectTaggingActionTest, Constructor) {
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
}

TEST_F(S3PutObjectTaggingActionTest, ValidateRequest) {
  MockObjectTagsStr =
      "<Tagging xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<TagSet><Tag><Key>organization124</Key>"
      "<Value>marketing123</Value>"
      "</Tag><Tag><Key>organization1234</Key>"
      "<Value>marketing123</Value></Tag></TagSet></Tagging>";
  call_count_one = 0;
  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockObjectTagsStr));
  action_under_test_ptr->clear_tasks();
  EXPECT_CALL(*(bucket_tag_body_factory_mock->mock_put_bucket_tag_body), isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*(bucket_tag_body_factory_mock->mock_put_bucket_tag_body),
              get_resource_tags_as_map())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockObjectTags));

  action_under_test_ptr->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test_ptr,
                         S3PutObjectTaggingActionTest::func_callback_one, this);
  action_under_test_ptr->validate_request();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectTaggingActionTest, ValidateInvalidRequest) {

  MockObjectTagsStr =
      "<Tagging xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<TagSet><Tag><Key>organization1234</Key>"
      "<Value>marketing123</Value>"
      "</Tag><Tag><Key>organization1234</Key>"
      "<Value>marketing123</Value></Tag></TagSet></Tagging>";

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockObjectTagsStr));
  EXPECT_CALL(*(bucket_tag_body_factory_mock->mock_put_bucket_tag_body), isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectTaggingActionTest, ValidateRequestMoreContent) {
  EXPECT_CALL(*request_mock, has_all_body_content()).Times(1).WillOnce(
      Return(false));
  EXPECT_CALL(*request_mock, get_data_length()).Times(1).WillOnce(Return(0));
  EXPECT_CALL(*request_mock, listen_for_incoming_data(_, _)).Times(1);
  action_under_test_ptr->clear_tasks();

  action_under_test_ptr->validate_request();
}

TEST_F(S3PutObjectTaggingActionTest, FetchBucketInfo) {
  CREATE_BUCKET_METADATA;
  EXPECT_TRUE(action_under_test_ptr->bucket_metadata != NULL);
}

TEST_F(S3PutObjectTaggingActionTest, FetchBucketInfoFailedNoSuchBucket) {
  CREATE_BUCKET_METADATA;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();

  EXPECT_TRUE(action_under_test_ptr->bucket_metadata != NULL);
  EXPECT_STREQ("NoSuchBucket",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectTaggingActionTest, FetchBucketInfoFailedInternalError) {
  CREATE_BUCKET_METADATA;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();

  EXPECT_TRUE(action_under_test_ptr->bucket_metadata != NULL);
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectTaggingActionTest, GetObjectMetadataFailedMissing) {
  CREATE_BUCKET_METADATA;
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_object_info_failed();

  EXPECT_TRUE(action_under_test_ptr->object_metadata != NULL);
  EXPECT_STREQ("NoSuchKey", action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectTaggingActionTest, GetObjectMetadataFailedInternalError) {
  CREATE_BUCKET_METADATA;
  CREATE_OBJECT_METADATA;
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));

  action_under_test_ptr->object_list_oid = {0xffff, 0xff1f};
  action_under_test_ptr->fetch_object_info_failed();

  EXPECT_TRUE(action_under_test_ptr->object_metadata != NULL);
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectTaggingActionTest, SetTag) {
  CREATE_BUCKET_METADATA;
  CREATE_OBJECT_METADATA;
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_tags(_))
      .Times(1);
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), save_metadata(_, _))
      .Times(1);
  action_under_test_ptr->save_tags_to_object_metadata();

  EXPECT_TRUE(action_under_test_ptr->object_metadata != NULL);
}

TEST_F(S3PutObjectTaggingActionTest, SetTagFailedInternalError) {
  CREATE_OBJECT_METADATA;
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->save_tags_to_object_metadata_failed();

  EXPECT_TRUE(action_under_test_ptr->object_metadata != NULL);
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectTaggingActionTest, SendResponseToClientServiceUnavailable) {
  CREATE_BUCKET_METADATA;

  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*request_mock, pause()).Times(1);
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(503, _)).Times(AtLeast(1));
  action_under_test_ptr->check_shutdown_and_rollback();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test_ptr->get_s3_error_code().c_str());
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutObjectTaggingActionTest, SendResponseToClientInternalError) {
  action_under_test_ptr->set_s3_error("InternalError");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutObjectTaggingActionTest, SendResponseToClientSuccess) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::saved));
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}


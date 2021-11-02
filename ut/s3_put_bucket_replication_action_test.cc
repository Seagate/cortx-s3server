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
#include "s3_put_bucket_replication_action.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::ReturnRef;

class S3PutBucketReplicationActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3PutBucketReplicationActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";

    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*request_mock, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));

    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(request_mock);
    bucket_replication_body_factory_mock =
        std::make_shared<MockS3PutReplicationBodyFactory>(
            MockReplicationConfigStr, MockRequestId);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*request_mock, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test_ptr = std::make_shared<S3PutBucketReplicationAction>(
        request_mock, bucket_meta_factory,
        bucket_replication_body_factory_mock);
    MockRequestId.assign("MockRequestId");
    MockReplicationConfigStr.assign("MockBucketReplication");
  }

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<S3PutBucketReplicationAction> action_under_test_ptr;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3PutReplicationBodyFactory>
      bucket_replication_body_factory_mock;
  std::string MockBucketReplicationConfig;
  std::string MockReplicationConfigStr;
  std::string MockRequestId;
  int call_count_one{0};
  std::string bucket_name;
  std::vector<std::string> mock_bucket_names;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PutBucketReplicationActionTest, Constructor) {
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
  EXPECT_TRUE(action_under_test_ptr->put_bucket_replication_body_factory !=
              nullptr);
}

TEST_F(S3PutBucketReplicationActionTest, ValidateRequest) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>role-string</Role><Rule><Status>Enabled</Status>"
      "<Priority>1</Priority><ID>Rule-1</ID><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication><Destination>"
      "<Bucket>dest-bucket</Bucket></Destination><Filter><And><Prefix/>"
      "<Tag><Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule>"
      "<Rule><Status>Enabled</Status><Priority>2</Priority>"
      "<ID>Rule-2</ID><DeleteMarkerReplication><Status>Disabled</Status>"
      "</DeleteMarkerReplication><Destination>"
      "<Bucket>dest-bucket</Bucket></Destination><Filter><And><Prefix /><Tag>"
      "<Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule></"
      "ReplicationConfiguration>";
  call_count_one = 0;
  mock_bucket_names.push_back("dest-bucket");

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      get_destination_bucket_list())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(mock_bucket_names));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      get_replication_configuration_as_json())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(MockBucketReplicationConfig));

  action_under_test_ptr->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test_ptr,
                         S3PutBucketReplicationActionTest::func_callback_one,
                         this);
  action_under_test_ptr->validate_request();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutBucketReplicationActionTest, ValidateRequestMoreContent) {
  EXPECT_CALL(*request_mock, has_all_body_content()).Times(1).WillOnce(
      Return(false));
  EXPECT_CALL(*request_mock, get_data_length()).Times(1).WillOnce(Return(0));
  EXPECT_CALL(*request_mock, listen_for_incoming_data(_, _)).Times(1);

  action_under_test_ptr->validate_request();
}
// Status is wrong
TEST_F(S3PutBucketReplicationActionTest, ValidateInvalidRequest) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>role-string</Role><Rule><Status>trr</Status>"
      "<Priority>1</Priority><ID>Rule-1</ID><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication><Destination>"
      "<Bucket>dest-bucket</Bucket></Destination><Filter><And><Prefix/>"
      "<Tag><Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule>"
      "<Rule><Status>Enabled</Status><Priority>2</Priority>"
      "<ID>Rule-2</ID><DeleteMarkerReplication><Status>Disabled</Status>"
      "</DeleteMarkerReplication><Destination>"
      "<Bucket>dest-bucket</Bucket></Destination><Filter><And><Prefix /><Tag>"
      "<Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule></"
      "ReplicationConfiguration>";
  mock_bucket_names.push_back("dest-bucket");

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}
/*
// Invalid request with duplicate rule Priority
TEST_F(S3PutBucketReplicationActionTest,
       ValidateInvalidRequestWithDuplicatePriority) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>role-string</Role><Rule><Status>Enabled</Status>"
      "<Priority>1</Priority><ID>Rule-1</ID><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication><Destination>"
      "<Bucket>dest-bucket</Bucket></Destination><Filter><And><Prefix/>"
      "<Tag><Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule>"
      "<Rule><Status>Enabled</Status><Priority>1</Priority>"
      "<ID>Rule-2</ID><DeleteMarkerReplication><Status>Disabled</Status>"
      "</DeleteMarkerReplication><Destination>"
      "<Bucket>dest-bucket</Bucket></Destination><Filter><And><Prefix /><Tag>"
      "<Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule></"
      "ReplicationConfiguration>";

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("InvalidArgumentDuplicateRulePriority",
               action_under_test_ptr->get_s3_error_code().c_str());
}

// Invalid request with duplicate rule -id
TEST_F(S3PutBucketReplicationActionTest,
       ValidateInvalidRequestWithDuplicateRuleID) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>2</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>";

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("InvalidArgumentDuplicateRuleID",
               action_under_test_ptr->get_s3_error_code().c_str());
}

// Invalid request- If Tag present in filter then DeleteMarkerReplication must
// be disable
TEST_F(S3PutBucketReplicationActionTest, ValidateInvalidRequestForTag) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>";

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("InvalidRequestForTagFilter",
               action_under_test_ptr->get_s3_error_code().c_str());
}

// Invalid request - If filter is added then deletemarker and priority must be
// present
TEST_F(S3PutBucketReplicationActionTest, ValidateInvalidRequestForFilter) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>";

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("InvalidRequestForFilter",
               action_under_test_ptr->get_s3_error_code().c_str());
}

// Invalid request with empty bucket name
TEST_F(S3PutBucketReplicationActionTest,
       ValidateInvalidRequestWithEmptyBucket) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>arn:aws:iam::456758429929:role/s3-replication-role</Role>"
      "<Rule><Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Destination><Bucket></Bucket>"
      "</Destination></Rule></ReplicationConfiguration>";
  ;

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("InvalidArgumentDestinationBucket",
               action_under_test_ptr->get_s3_error_code().c_str());
}
*/
TEST_F(S3PutBucketReplicationActionTest, SetReplicationConfig) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillOnce(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              set_bucket_replication_configuration(_)).Times(AtLeast(1));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), update(_, _))
      .Times(AtLeast(1));
  action_under_test_ptr->save_replication_config_to_bucket_metadata();
}

TEST_F(S3PutBucketReplicationActionTest,
       SetReplicationConfigWhenBucketMissing) {
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

TEST_F(S3PutBucketReplicationActionTest, SetReplicationConfigWhenBucketFailed) {
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

TEST_F(S3PutBucketReplicationActionTest,
       SetReplicationConfigWhenBucketFailedToLaunch) {
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

TEST_F(S3PutBucketReplicationActionTest,
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

TEST_F(S3PutBucketReplicationActionTest, SendResponseToClientMalformedXML) {
  action_under_test_ptr->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  action_under_test_ptr->set_s3_error("MalformedXML");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutBucketReplicationActionTest, SendResponseToClientNoSuchBucket) {
  action_under_test_ptr->set_s3_error("NoSuchBucket");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutBucketReplicationActionTest, SendResponseToClientSuccess) {
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PutBucketReplicationActionTest, SendResponseToClientInternalError) {
  action_under_test_ptr->set_s3_error("InternalError");
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

// The source and destination buckets cannot be the same
TEST_F(S3PutBucketReplicationActionTest,
       ValidateIfDestinationAndSourceNameIsSame) {

  MockReplicationConfigStr =
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>role-string</Role><Rule><Status>Enabled</Status>"
      "<Priority>1</Priority><ID>Rule-1</ID><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication><Destination>"
      "<Bucket>seagatebucket</Bucket></Destination>"
      "<Filter><And><Prefix></Prefix>"
      "<Tag><Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule>"
      "<Rule><Status>Enabled</Status><Priority>2</Priority>"
      "<ID>Rule-2</ID><DeleteMarkerReplication><Status>Disabled</Status>"
      "</DeleteMarkerReplication><Destination>"
      "<Bucket>seagatebucket</Bucket></Destination>"
      "<Filter><And><Prefix></Prefix>"
      "<Tag><Key>key12</Key><Value>vv</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter></Rule></"
      "ReplicationConfiguration>";
  mock_bucket_names.push_back(bucket_name);

  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(MockReplicationConfigStr));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      isOK())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(
      *(bucket_replication_body_factory_mock->mock_put_bucket_replication_body),
      get_destination_bucket_list())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(mock_bucket_names));

  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->validate_request();
  EXPECT_STREQ("InvalidRequestDestinationBucket",
               action_under_test_ptr->get_s3_error_code().c_str());
}

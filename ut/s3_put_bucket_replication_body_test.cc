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
#include <string>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_request_object.h"
#include "mock_s3_factory.h"
#include "s3_put_replication_body.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::ReturnRef;

class S3PutReplicationBodyTest : public testing::Test {
 protected:
  S3PutReplicationBodyTest() {
    put_bucket_replication_body_factory =
        std::make_shared<S3PutReplicationBodyFactory>();
  }
  bool result = false;
  std::string RequestId;
  std::string BucketRplicationContentStr;
  std::string replication_config_as_json;
  std::shared_ptr<S3PutReplicationBody> put_bucket_replication_body;
  std::shared_ptr<S3PutReplicationBodyFactory>
      put_bucket_replication_body_factory;
};

TEST_F(S3PutReplicationBodyTest, ValidateRequestBodyXml) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>arn:aws:iam::456758429929:role/s3-replication-role</Role>"
      "<Rule>"
      "<DeleteMarkerReplication>"
      "<Status>Enabled</Status>"
      "</DeleteMarkerReplication>"
      "<Destination>"
      "<Bucket>dest-bucket</Bucket>"
      "</Destination>"
      "<ID>Rule-1</ID>"
      "<Priority>1</Priority>"
      "<Status>Enabled</Status>"
      "</Rule>"
      "</ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

testing::AssertionResult compare_json(
    std::string request_replication_configuration,
    std::string replication_config_as_json) {
  if ((request_replication_configuration.compare(replication_config_as_json)) ==
      0)
    return testing::AssertionSuccess();
  else
    return testing::AssertionFailure();
}
// need changes
/*TEST_F(S3PutReplicationBodyTest, ValidateRequestCompareContents) {
  std::string request_replication_configuration;
  bool json_compare;
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");
  replication_config_as_json.assign(
      "{
      "\"ReplicationConfiguration\": {"
      "\"Role\": \"string\","
      "\"Rule\": {"
      "\"ID\": \"Rule-1\","
      "\"Status\": \"Enabled\","
      "\"Priority\": \"1\","
      "\"DeleteMarkerReplication\": {"
      "\"Status\": \"Enabled\""
      "},"
      "\"Destination\": {"
      "\"Bucket\": \"arn:aws:s3:::dstination-buckt\""
      "}"
      "}"
      "}"
      "}");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  request_replication_configuration =
      put_bucket_replication_body->get_replication_configuration_as_json();

  json_compare = (bucket_tags_map.size() == request_tags_map.size()) &&
                 (std::equal(bucket_tags_map.begin(), bucket_tags_map.end(),
                             request_tags_map.begin()));
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
  EXPECT_TRUE(compare_json(request_replication_configuration,
                           replication_config_as_json));
}*/

// Atleast one rule should be present
TEST_F(S3PutReplicationBodyTest, ValidateNumberOfRuleNodes) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Atleast One rule should be in enable state
TEST_F(S3PutReplicationBodyTest, ValidateAtleastOneRuleIsEnabled) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Disable</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule><Rule><ID>Rule-2</ID>"
      "<Status>Disable</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validated rule status value.It should be either Enable or Disable
TEST_F(S3PutReplicationBodyTest, ValidateRuleStatusValue) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Suspend</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validated DeleteReplication node status value.It should be either Enable or
// Disable
TEST_F(S3PutReplicationBodyTest, ValidateDeleteReplicationStatusValue) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Suspended</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validated DeleteReplication node status value.It should be either Enable or
// Disable
TEST_F(S3PutReplicationBodyTest,
       ValidateDeleteReplicationStatusValueAsDisable) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enable</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disable</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

// Validate if Destination node is present in rule
TEST_F(S3PutReplicationBodyTest, ValidateDestinationNode) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Suspend</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disable</Status></DeleteMarkerReplication>"
      "<Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate Empty Destination node is present in rule
TEST_F(S3PutReplicationBodyTest, ValidateEmptyDestinationNode) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Suspend</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disable</Status></DeleteMarkerReplication>"
      "<Destination>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

TEST_F(S3PutReplicationBodyTest, ValidateEmptyBucketNode) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Disable</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disable</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>arn:aws:s3:::dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}
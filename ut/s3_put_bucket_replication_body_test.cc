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
  S3PutReplicationBodyTest()
      : put_bucket_replication_body_factory(
            std::make_shared<S3PutReplicationBodyFactory>()) {}
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
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
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

// Validated rule status value.It should be either Enable or Disable
TEST_F(S3PutReplicationBodyTest, ValidateRuleStatusValue) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Suspend</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Skip rule status parameter.It is mandatory field.
TEST_F(S3PutReplicationBodyTest, ValidateEmptyRuleStatusValue) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status></Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
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
      "<Destination><Bucket>destination-bucket</Bucket>"
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
TEST_F(S3PutReplicationBodyTest, ValidateDeleteReplicationInvalidStatusValue) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>arn:aws:iam::456758429929:role/s3-replication-role</Role>"
      "<Rule>"
      "<DeleteMarkerReplication>"
      "<Status>gtru</Status>"
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
      "<Destination><Bucket></Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate request against skipped rule id
TEST_F(S3PutReplicationBodyTest, ValidateRequestSkippedRuleID) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

// Validate request against skipped And node.
// And is required if more than one fields are present in the Filter
TEST_F(S3PutReplicationBodyTest, ValidateRequestSkippedAndNode) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule>"
      "<Status>Enabled</Status><ID>Rule-1</ID>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate request in case of only Prefix is present in the filter along with
// And node
TEST_F(S3PutReplicationBodyTest, InvalidAndNode) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate request with only Prefix present in the Filter
// Prefix can be empty
TEST_F(S3PutReplicationBodyTest, ValidateRequestForPrefix) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><Prefix></Prefix></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

// Validate request with only one Tag present in the Filter
TEST_F(S3PutReplicationBodyTest, ValidateRequestWithOneTag) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

// Validate request with two Tag present without and node in the Filter
TEST_F(S3PutReplicationBodyTest, InvalidTagNodes) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate request with two same key in filter
TEST_F(S3PutReplicationBodyTest, ValidateDuplicateKeyRequest) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key1</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate request Skip Key node in filter
TEST_F(S3PutReplicationBodyTest, InvalidKeyRequest) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag>"
      "<Value>value1</Value></Tag><Tag><Key>key1</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate Empty key request in filter
TEST_F(S3PutReplicationBodyTest, InvalidEmptyKeyRequest) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key></Key>"
      "<Value>value1</Value></Tag><Tag><Key>key1</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate Empty filter
TEST_F(S3PutReplicationBodyTest, InvalidEmptyFilter) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Filter></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

// if Tag present in filter then DeleteMarkerReplication must be disable
TEST_F(S3PutReplicationBodyTest, ValidateRequestForTag) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Enabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "</Rule><Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// If filter is added then deletemarker and priority must be present
TEST_F(S3PutReplicationBodyTest, ValidateRequestForFilter) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/ \">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "</Rule><Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}
// Validate request for multiple rules
TEST_F(S3PutReplicationBodyTest, ValidateRequestForMultipleRules) {
  BucketRplicationContentStr.assign(
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
      "</Destination></Rule><Rule><ID>Rule-2</ID>"
      "<Status>Enabled</Status>"
      "<Priority>2</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

// Validate request for duplicate key in multiple rules
TEST_F(S3PutReplicationBodyTest,
       ValidateRequestForDuplicateKeyInMultipleRules) {
  BucketRplicationContentStr.assign(
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
      "</Destination></Rule><Rule><ID>Rule-2</ID>"
      "<Status>Enabled</Status>"
      "<Priority>2</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}
// Validate request for Duplicate rule id
TEST_F(S3PutReplicationBodyTest, ValidateDuplicateRuleIDRequest) {
  BucketRplicationContentStr.assign(
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
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key3</Key>"
      "<Value>value3</Value></Tag><Tag><Key>key4</Key>"
      "<Value>value4</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Validate request for Duplicate Priority
TEST_F(S3PutReplicationBodyTest, ValidateDuplicateRulePriority) {
  BucketRplicationContentStr.assign(
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
      "</Destination></Rule><Rule><ID>Rule-2</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key4</Key>"
      "<Value>value4</Value></Tag><Tag><Key>key3</Key>"
      "<Value>value3</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Storage Class is not supported
TEST_F(S3PutReplicationBodyTest, ValidateStorageClassNotSupported) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "<StorageClass>STANDARD</StorageClass>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// EncryptionConfiguration is not supported
TEST_F(S3PutReplicationBodyTest, ValidateEncryptionConfigurationNotSupported) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "<EncryptionConfiguration>"
      "<ReplicaKmsKeyID>ARN</ReplicaKmsKeyID>"
      "</EncryptionConfiguration>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// AccessControlTranslation is not implemented
TEST_F(S3PutReplicationBodyTest,
       ValidateAccessControlTranslationNotImplemented) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "<AccessControlTranslation>"
      "<Owner>Destination</Owner>"
      "</AccessControlTranslation>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Account is not implemented
TEST_F(S3PutReplicationBodyTest, ValidateAccountNotImplemented) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "<Account>destination-bucket-owner-account-id</Account>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// Metrics is not implemented
TEST_F(S3PutReplicationBodyTest, ValidateMetricsNotImplemented) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "<Metrics>"
      "<EventThreshold>"
      "<Minutes>10</Minutes>"
      "</EventThreshold>"
      "<Status>Enabled</Status>"
      "</Metrics>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// ReplicationTime is not implemented
TEST_F(S3PutReplicationBodyTest, ValidateReplicationTimeNotImplemented) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "<ReplicationTime>"
      "<Status>Enabled</Status>"
      "<Time>"
      "<Minutes>10</Minutes>"
      "</Time>"
      "</ReplicationTime>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// ExistingObjectReplication is not implemented
TEST_F(S3PutReplicationBodyTest,
       ValidateExistingObjectReplicationNotImplemented) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<ExistingObjectReplication>"
      "<Status>Enabled</Status>"
      "</ExistingObjectReplication>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// SourceSelectionCriteria is not implemented
TEST_F(S3PutReplicationBodyTest,
       ValidateSourceSelectionCriteriaNotImplemented) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule><ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Priority>1</Priority><DeleteMarkerReplication>"
      "<Status>Disabled</Status></DeleteMarkerReplication>"
      "<Filter><And><Prefix>abc</Prefix><Tag><Key>key1</Key>"
      "<Value>value1</Value></Tag><Tag><Key>key2</Key>"
      "<Value>value2</Value></Tag></And></Filter>"
      "<SourceSelectionCriteria>"
      "<ReplicaModifications>"
      "<Status>Enabled</Status>"
      "</ReplicaModifications>"
      "<SseKmsEncryptedObjects>"
      "<Status>Enabled</Status>"
      "</SseKmsEncryptedObjects>"
      "</SourceSelectionCriteria>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_FALSE(result);
}

// AWS still supports the old v1 API
TEST_F(S3PutReplicationBodyTest, ValidateV1Request) {
  BucketRplicationContentStr.assign(
      "<ReplicationConfiguration "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<Role>Replication_Role</Role><Rule>"
      "<ID>Rule-1</ID>"
      "<Status>Enabled</Status>"
      "<Prefix>abc</Prefix>"
      "<Destination><Bucket>dstination-bucket</Bucket>"
      "</Destination></Rule></ReplicationConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_replication_body =
      put_bucket_replication_body_factory->create_put_replication_body(
          BucketRplicationContentStr, RequestId);
  result = put_bucket_replication_body->isOK();
  EXPECT_TRUE(result);
}

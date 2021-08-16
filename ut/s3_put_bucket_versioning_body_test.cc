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
/*
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_request_object.h"
#include "mock_s3_factory.h"
#include "s3_put_versioning_body.h"

class S3PutVersioningBodyTest : public testing::Test {
 protected:
  S3PutVersioningBodyTest() {
    put_bucket_versioning_body_factory =
        std::make_shared<S3PutBucketVersioningBodyFactory>();
  }
  bool result = false;
  std::string RequestId;
  std::string BucketVersionStr;
  std::string bucket_versioning_status;
  std::shared_ptr<S3PutVersioningBody> put_bucket_versioning_body;
  std::shared_ptr<S3PutBucketVersioningBodyFactory>
      put_bucket_versioning_body_factory;
};

TEST_F(S3PutVersioningBodyTest, ValidateRequestBodyXml) {
  BucketVersionStr.assign("<VersioningConfiguration xmlns=" http
                          :  // s3.amazonaws.com/doc/2006-03-01/">"
                          "<Status>Enabled</Status>"
                          "</VersioningConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_versioning_body =
      put_bucket_versioning_body_factory->create_put_resource_versioning_body(
          BucketVersionStr, RequestId);
  result = put_bucket_versioning_body->isOK();
  EXPECT_TRUE(result);
}

TEST_F(S3PutVersioningBodyTest, ValidateVersioningEnabledRequest) {
  std::string request_versioning_status;
  BucketTagsStr.assign("<VersioningConfiguration xmlns=" http
                       :  // s3.amazonaws.com/doc/2006-03-01/">"
                       "<Status>Enabled</Status>"
                       "</VersioningConfiguration>");
  RequestId.assign("RequestId");
  bucket_versioning_status.assign("Enabled");

  put_bucket_versioning_body =
      put_bucket_versioning_body_factory->create_put_resource_versioning_body(
          BucketVersionStr, RequestId);
  request_versioning_status =
      put_bucket_versioning_body->get_versioning_status();

  testing::AssertionResult compare_versioning_status(
      std::string request_versioning_status) {
    if ((request_versioning_status.compare(bucket_versioning_status)) == 0)
      return testing::AssertionSuccess();
    else
      return testing::AssertionFailure();
  }

  result = put_bucket_versioning_body->isOK();
  EXPECT_TRUE(result);
  EXPECT_TRUE(compare_versioning_status(request_versioning_status));
}

TEST_F(S3PutVersioningBodyTest, ValidateVersioningSuspendedRequest) {
  std::string request_versioning_status;
  BucketTagsStr.assign("<VersioningConfiguration xmlns=" http
                       :  // s3.amazonaws.com/doc/2006-03-01/">"
                       "<Status>Suspended</Status>"
                       "</VersioningConfiguration>");
  RequestId.assign("RequestId");
  bucket_versioning_status.assign("Suspended");

  put_bucket_versioning_body =
      put_bucket_versioning_body_factory->create_put_resource_versioning_body(
          BucketVersionStr, RequestId);
  request_versioning_status =
      put_bucket_versioning_body->get_versioning_status();

  testing::AssertionResult compare_versioning_status(
      std::string request_versioning_status) {
    if ((request_versioning_status.compare(bucket_versioning_status)) == 0)
      return testing::AssertionSuccess();
    else
      return testing::AssertionFailure();
  }

  result = put_bucket_versioning_body->isOK();
  EXPECT_TRUE(result);
  EXPECT_TRUE(compare_versioning_status(request_versioning_status));
}

TEST_F(S3PutTagBodyTest, ValidateEmptyVersioningConfigurationXmlTag) {
  BucketTagsStr.assign("< xmlns=" http
                       :  // s3.amazonaws.com/doc/2006-03-01/">"
                       "<Status></Status>");
  RequestId.assign("RequestId");

  put_bucket_versioning_body =
      put_bucket_versioning_body_factory->create_put_resource_versioning_body(
          BucketVersionStr, RequestId);
  result = put_bucket_versioning_body->isOK();
  EXPECT_FALSE(result);
}

TEST_F(S3PutTagBodyTest, ValidateEmptyStatusXmlTag) {
  BucketTagsStr.assign("<VersioningConfiguration xmlns=" http
                       :  // s3.amazonaws.com/doc/2006-03-01/">"
                       "<Status></Status>"
                       "</VersioningConfiguration>");
  RequestId.assign("RequestId");

  put_bucket_versioning_body =
      put_bucket_versioning_body_factory->create_put_resource_versioning_body(
          BucketVersionStr, RequestId);
  result = put_bucket_versioning_body->isOK();
  EXPECT_FALSE(result);
}

TEST_F(S3PutTagBodyTest, ValidateUnversionedStatus) {
  BucketTagsStr.assign("<VersioningConfiguration xmlns=" http
                       :  // s3.amazonaws.com/doc/2006-03-01/">"
                       "<Status>Unversioned</Status>"
                       "</VersioningConfiguration>");
  RequestId.assign("RequestId");
  bucket_versioning_status.assign("Unversioned");

  put_bucket_versioning_body =
      put_bucket_versioning_body_factory->create_put_resource_versioning_body(
          BucketVersionStr, RequestId);

  result =
      put_bucket_tag_body->validate_bucket_xml_tags(bucket_versioning_status);
  EXPECT_TRUE(result);
}
*/
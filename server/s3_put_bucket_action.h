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

#pragma once

#ifndef __S3_SERVER_S3_PUT_BUCKET_ACTION_H__
#define __S3_SERVER_S3_PUT_BUCKET_ACTION_H__

#include <tuple>
#include <vector>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"
#include "s3_put_bucket_body.h"

#define BUCKETNAME_MIN_LENGTH 3
#define BUCKETNAME_MAX_LENGTH 63

class S3PutBucketAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3PutBucketBodyFactory> put_bucketbody_factory;
  std::shared_ptr<S3PutBucketBody> put_bucket_body;

  std::string request_content;
  std::string location_constraint;  // Received in request body.

 public:
  S3PutBucketAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory =
          nullptr,
      std::shared_ptr<S3PutBucketBodyFactory> bucket_body_factory = nullptr);

  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void validate_request_body(std::string content);
  // AWS recommends that all bucket names comply with DNS naming convention
  // See Bucket naming restrictions in below link.
  // https://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
  void validate_bucket_name();
  void read_metadata();
  void create_bucket();
  void create_bucket_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutBucketActionTest, Constructor);
  FRIEND_TEST(S3PutBucketActionTest, ValidateRequest);
  FRIEND_TEST(S3PutBucketActionTest, ValidateRequestInvalid);
  FRIEND_TEST(S3PutBucketActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest1);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest2);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest3);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest4);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest5);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest6);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest1);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest2);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest3);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest4);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest5);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest6);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest7);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest8);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest9);
  FRIEND_TEST(S3PutBucketActionTest, ReadMetaData);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketAlreadyExist);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketAlreadyExistForAccount);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketSuccess);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketFailed);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientMalformedXML);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientInternalError);
};

#endif

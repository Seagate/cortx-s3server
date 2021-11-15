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

#pragma once

#ifndef __S3_SERVER_S3_PUT_BUCKET_REPLICATION_ACTION_H__
#define __S3_SERVER_S3_PUT_BUCKET_REPLICATION_ACTION_H__

#include <memory>
#include <string>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3PutBucketReplicationAction : public S3BucketAction {
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3PutReplicationBodyFactory>
      put_bucket_replication_body_factory;
  std::shared_ptr<S3PutReplicationBody> put_bucket_replication_body;

  std::string new_bucket_replication_content;
  std::string replication_config_json;

 public:
  S3PutBucketReplicationAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3PutReplicationBodyFactory> bucket_body_factory =
          nullptr);

  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void validate_request_body(std::string content);
  void save_replication_config_to_bucket_metadata();
  void save_replication_configuration_to_bucket_metadata_failed();
  void fetch_bucket_info_failed();
  void send_response_to_s3_client();
  // void fetch_additional_bucket_info_failed();

  // For Testing purpose
  FRIEND_TEST(S3PutBucketReplicationActionTest, Constructor);
  FRIEND_TEST(S3PutBucketReplicationActionTest, ValidateRequest);
  FRIEND_TEST(S3PutBucketReplicationActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutBucketReplicationActionTest, ValidateInvalidRequest);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              ValidateInvalidRequestWithDuplicatePriority);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              ValidateInvalidRequestWithDuplicateRuleID);
  FRIEND_TEST(S3PutBucketReplicationActionTest, ValidateInvalidRequestForTag);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              ValidateInvalidRequestWithEmptyBucket);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              ValidateInvalidRequestForFilter);
  FRIEND_TEST(S3PutBucketReplicationActionTest, SetReplicationConfig);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SetReplicationConfigWhenBucketMissing);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SetReplicationConfigWhenBucketFailed);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SetReplicationConfigWhenBucketFailedToLaunch);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SendResponseToClientMalformedXML);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3PutBucketReplicationActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PutBucketReplicationActionTest,
              SendResponseToClientInternalError);
};

#endif

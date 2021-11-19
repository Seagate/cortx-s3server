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

#ifndef __S3_SERVER_S3_PUT_BUCKET_VERSION_ACTION_H__
#define __S3_SERVER_S3_PUT_BUCKET_VERSION_ACTION_H__

#include <memory>
#include <string>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3PutBucketVersioningAction : public S3BucketAction {
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3PutBucketVersioningBodyFactory> put_bucket_version_factory;
  std::shared_ptr<S3PutVersioningBody> put_bucket_version_body;

  std::string bucket_content_string;
  std::string bucket_version_status;

 public:
  S3PutBucketVersioningAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3PutBucketVersioningBodyFactory> bucket_body_factory =
          nullptr);

  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void validate_request_body(const std::string& content);
  void validate_request_xml_versioning_status();
  void save_versioning_status_to_bucket_metadata();
  void save_versioning_status_to_bucket_metadata_failed();
  void fetch_bucket_info_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutBucketVersioningActionTest, Constructor);
  FRIEND_TEST(S3PutBucketVersioningActionTest, ValidateRequest);
  FRIEND_TEST(S3PutBucketVersioningActionTest, ValidateInvalidRequest);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              ValidateRequestXmlVersioningConfiguration);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              ValidateRequestXmlWithSuspendedState);
  FRIEND_TEST(S3PutBucketVersioningActionTest, ValidateInvalidRequestXmlTags);
  FRIEND_TEST(S3PutBucketVersioningActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutBucketVersioningActionTest, SetVersioningState);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SetVersioningStateWhenBucketMissing);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SetVersioningStateWhenBucketFailed);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SetVersioningStateWhenBucketFailedToLaunch);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SendResponseToClientMalformedXML);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3PutBucketVersioningActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PutBucketVersioningActionTest,
              SendResponseToClientInternalError);
};

#endif

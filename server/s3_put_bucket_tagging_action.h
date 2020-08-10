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

#ifndef __S3_SERVER_S3_PUT_BUCKET_TAG_ACTION_H__
#define __S3_SERVER_S3_PUT_BUCKET_TAG_ACTION_H__

#include <memory>
#include <string>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3PutBucketTaggingAction : public S3BucketAction {
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3PutTagsBodyFactory> put_bucket_tag_body_factory;
  std::shared_ptr<S3PutTagBody> put_bucket_tag_body;

  std::string new_bucket_tags;
  std::map<std::string, std::string> bucket_tags_map;

 public:
  S3PutBucketTaggingAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3PutTagsBodyFactory> bucket_body_factory = nullptr);

  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void validate_request_body(std::string content);
  void validate_request_xml_tags();
  void save_tags_to_bucket_metadata();
  void save_tags_to_bucket_metadata_failed();
  void fetch_bucket_info_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutBucketTaggingActionTest, Constructor);
  FRIEND_TEST(S3PutBucketTaggingActionTest, ValidateRequest);
  FRIEND_TEST(S3PutBucketTaggingActionTest, ValidateInvalidRequest);
  FRIEND_TEST(S3PutBucketTaggingActionTest, ValidateRequestXmlTags);
  FRIEND_TEST(S3PutBucketTaggingActionTest, ValidateInvalidRequestXmlTags);
  FRIEND_TEST(S3PutBucketTaggingActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SetTags);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SetTagsWhenBucketMissing);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SetTagsWhenBucketFailed);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SetTagsWhenBucketFailedToLaunch);
  FRIEND_TEST(S3PutBucketTaggingActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SendResponseToClientMalformedXML);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PutBucketTaggingActionTest, SendResponseToClientInternalError);
};

#endif

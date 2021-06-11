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

#ifndef __S3_SERVER_S3_PUT_OBJECT_TAGGING_ACTION_H__
#define __S3_SERVER_S3_PUT_OBJECT_TAGGING_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>

#include "s3_factory.h"
#include "s3_object_action_base.h"
#include "s3_object_metadata.h"

class S3PutObjectTaggingAction : public S3ObjectAction {
  std::shared_ptr<S3PutTagsBodyFactory> put_object_tag_body_factory;
  std::shared_ptr<S3PutTagBody> put_object_tag_body;

  std::string new_object_tags;
  std::map<std::string, std::string> object_tags_map;

 public:
  S3PutObjectTaggingAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3PutTagsBodyFactory> object_body_factory = nullptr);

  void setup_steps();
  void validate_request();
  void validate_request_xml_tags();
  void consume_incoming_content();
  void validate_request_body(std::string content);
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void save_tags_to_object_metadata();
  void save_tags_to_object_metadata_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutObjectTaggingActionTest, Constructor);
  FRIEND_TEST(S3PutObjectTaggingActionTest, ValidateRequest);
  FRIEND_TEST(S3PutObjectTaggingActionTest, ValidateInvalidRequest);
  FRIEND_TEST(S3PutObjectTaggingActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutObjectTaggingActionTest, FetchBucketInfo);
  FRIEND_TEST(S3PutObjectTaggingActionTest, FetchBucketInfoFailedNoSuchBucket);
  FRIEND_TEST(S3PutObjectTaggingActionTest, FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3PutObjectTaggingActionTest, GetObjectMetadataFailedMissing);
  FRIEND_TEST(S3PutObjectTaggingActionTest,
              GetObjectMetadataFailedInternalError);
  FRIEND_TEST(S3PutObjectTaggingActionTest, SetTag);
  FRIEND_TEST(S3PutObjectTaggingActionTest, SetTagFailedMissing);
  FRIEND_TEST(S3PutObjectTaggingActionTest, SetTagFailedInternalError);
  FRIEND_TEST(S3PutObjectTaggingActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutObjectTaggingActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3PutObjectTaggingActionTest, SendResponseToClientSuccess);
};

#endif

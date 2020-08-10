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

#ifndef __S3_SERVER_S3_DELETE_OBJECT_TAGGING_ACTION_H__
#define __S3_SERVER_S3_DELETE_OBJECT_TAGGING_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>

#include "s3_factory.h"
#include "s3_object_action_base.h"
#include "s3_object_metadata.h"

class S3DeleteObjectTaggingAction : public S3ObjectAction {

 public:
  S3DeleteObjectTaggingAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  void setup_steps();
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void delete_object_tags();
  void delete_object_tags_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, Constructor);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, FetchBucketInfo);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              FetchBucketInfoFailedNoSuchBucket);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, GetObjectMetadataFailedMissing);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              GetObjectMetadataFailedInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, DeleteTag);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, DeleteTagFailed);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, DeleteTagFailedInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              SendResponseToClientInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, SendResponseToClientSuccess);
};

#endif

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

#ifndef __S3_SERVER_S3_HEAD_BUCKET_ACTION_H__
#define __S3_SERVER_S3_HEAD_BUCKET_ACTION_H__

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3HeadBucketAction : public S3BucketAction {

 public:
  S3HeadBucketAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr);

  void setup_steps();
  void fetch_bucket_info_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3HeadBucketActionTest, Constructor);
  FRIEND_TEST(S3HeadBucketActionTest, ReadMetaDataFailedTest1);
  FRIEND_TEST(S3HeadBucketActionTest, ReadMetaDataFailedTest2);
  FRIEND_TEST(S3HeadBucketActionTest, ReadMetaDataFailedTest3);
  FRIEND_TEST(S3HeadBucketActionTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3HeadBucketActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3HeadBucketActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3HeadBucketActionTest, SendResponseToClientSuccess);
};

#endif

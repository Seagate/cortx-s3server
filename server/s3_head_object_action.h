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

#ifndef __S3_SERVER_S3_HEAD_OBJECT_ACTION_H__
#define __S3_SERVER_S3_HEAD_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_object_action_base.h"

class S3HeadObjectAction : public S3ObjectAction {

 public:
  S3HeadObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  void setup_steps();

  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void send_response_to_s3_client();

  FRIEND_TEST(S3HeadObjectActionTest, ConstructorTest);
  FRIEND_TEST(S3HeadObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoWhenBucketFetchFailed);
  FRIEND_TEST(S3HeadObjectActionTest,
              FetchObjectInfoWhenBucketFetchFailedToLaunch);
  FRIEND_TEST(S3HeadObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3HeadObjectActionTest,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoReturnedMissing);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoFailedWithError);
  FRIEND_TEST(S3HeadObjectActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3HeadObjectActionTest, SendErrorResponse);
  FRIEND_TEST(S3HeadObjectActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3HeadObjectActionTest, SendSuccessResponse);
};

#endif

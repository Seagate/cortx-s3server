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

#ifndef __S3_SERVER_S3_DELETE_BUCKET_POLICY_ACTION_H__
#define __S3_SERVER_S3_DELETE_BUCKET_POLICY_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_kvs_reader.h"
#include "s3_factory.h"

class S3DeleteBucketPolicyAction : public S3BucketAction {

  // Helpers
  std::string get_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name();
  }

 public:
  S3DeleteBucketPolicyAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr);

  void setup_steps();

  void fetch_bucket_info_failed();

  void delete_bucket_policy();
  void delete_bucket_policy_successful();
  void delete_bucket_policy_failed();

  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3DeleteBucketPolicyActionTest, DeleteBucketPolicy);
  FRIEND_TEST(S3DeleteBucketPolicyActionTest, DeleteBucketPolicySuccessful);
  FRIEND_TEST(S3DeleteBucketPolicyActionTest, DeleteBucketPolicyFailed);
  FRIEND_TEST(S3DeleteBucketPolicyActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3DeleteBucketPolicyActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3DeleteBucketPolicyActionTest,
              SendResponseToClientInternalError);
};

#endif

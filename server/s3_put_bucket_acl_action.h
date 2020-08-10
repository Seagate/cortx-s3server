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

#ifndef __S3_SERVER_S3_PUT_BUCKET_ACL_ACTION_H__
#define __S3_SERVER_S3_PUT_BUCKET_ACL_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>

#include "s3_bucket_action_base.h"
#include "s3_factory.h"

class S3PutBucketACLAction : public S3BucketAction {

  std::string user_input_acl;

 public:
  S3PutBucketACLAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr);

  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void fetch_bucket_info_failed();
  void on_aclvalidation_success();
  void on_aclvalidation_failure();
  void validate_acl_with_auth();
  void setvalidateacl();
  void setacl();
  void setacl_failed();
  void send_response_to_s3_client();

  FRIEND_TEST(S3PutBucketAclActionTest, Constructor);
  FRIEND_TEST(S3PutBucketAclActionTest, FetchBucketInfo);
  FRIEND_TEST(S3PutBucketAclActionTest, FetchBucketInfoFailedWithMissing);
  FRIEND_TEST(S3PutBucketAclActionTest, FetchBucketInfoFailed);
  FRIEND_TEST(S3PutBucketAclActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3PutBucketAclActionTest, SendErrorResponse);
  FRIEND_TEST(S3PutBucketAclActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3PutBucketAclActionTest, SetAclShouldUpdateMetadata);
  FRIEND_TEST(S3PutBucketAclActionTest, ValidateOnAllDataShouldCallNext);
  FRIEND_TEST(S3PutBucketAclActionTest, ValidateOnPartialDataShouldWaitForMore);
};

#endif

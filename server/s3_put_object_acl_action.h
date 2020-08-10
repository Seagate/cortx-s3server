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

#ifndef __S3_SERVER_S3_PUT_OBJECT_ACL_ACTION_H__
#define __S3_SERVER_S3_PUT_OBJECT_ACL_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>

#include "s3_object_action_base.h"

class S3PutObjectACLAction : public S3ObjectAction {
  // std::string user_input_acl;

 public:
  S3PutObjectACLAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);
  std::string user_input_acl;
  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void on_aclvalidation_success();
  void on_aclvalidation_failure();
  void validate_acl_with_auth();
  void setvalidateacl();
  void set_authorization_meta();
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void setacl();
  void setacl_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutObjectACLActionTest, Constructor);
  FRIEND_TEST(S3PutObjectACLActionTest, ValidateRequest);
  FRIEND_TEST(S3PutObjectACLActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutObjectACLActionTest, FetchBucketInfo);
  FRIEND_TEST(S3PutObjectACLActionTest, FetchBucketInfoFailedNoSuchBucket);
  FRIEND_TEST(S3PutObjectACLActionTest, FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3PutObjectACLActionTest, GetObjectMetadataEmpty);
  FRIEND_TEST(S3PutObjectACLActionTest, GetObjectMetadata);
  FRIEND_TEST(S3PutObjectACLActionTest, GetObjectMetadataFailedMissing);
  FRIEND_TEST(S3PutObjectACLActionTest, GetObjectMetadataFailedInternalError);
  FRIEND_TEST(S3PutObjectACLActionTest, Setacl);
  FRIEND_TEST(S3PutObjectACLActionTest, SetaclFailedMissing);
  FRIEND_TEST(S3PutObjectACLActionTest, SetaclFailedInternalError);
  FRIEND_TEST(S3PutObjectACLActionTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutObjectACLActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3PutObjectACLActionTest, SendResponseToClientSuccess);
};

#endif

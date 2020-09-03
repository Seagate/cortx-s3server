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

#ifndef __S3_SERVER_S3_OBJECT_ACTION_BASE_H__
#define __S3_SERVER_S3_OBJECT_ACTION_BASE_H__

#include <functional>
#include <memory>
#include <vector>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
/*
   All the object action classes (PUT, GET etc..) need to derive from
   S3ObjectAction class. This class provides metdata fetch functions
   required for object action authorization and object action processing

   Derived object action classes need to implement below functions
   fetch_bucket_info_failed(): handle failures if bucket metdata doesnt exist
   fetch_object_info_failed(): handle failures if object metdata doesnt exist

 */
class S3ObjectAction : public S3Action {

 protected:
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;

  m0_uint128 object_list_oid;
  m0_uint128 objects_version_list_oid;
  void fetch_bucket_info();
  void fetch_object_info();
  virtual void fetch_bucket_info_failed() = 0;
  virtual void fetch_object_info_failed() = 0;
  virtual void fetch_object_info_success();
  virtual void fetch_bucket_info_success();

  // Sets appropriate Fault points for any shutdown tests.
  void setup_fi_for_shutdown_tests();

 public:
  S3ObjectAction(std::shared_ptr<S3RequestObject> req,
                 std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory =
                     nullptr,
                 std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory =
                     nullptr,
                 bool check_shutdown = true,
                 std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
                 bool skip_auth = false);
  virtual ~S3ObjectAction();

  void load_metadata();
  virtual void set_authorization_meta();

  FRIEND_TEST(S3ObjectActionTest, Constructor);
  FRIEND_TEST(S3ObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3ObjectActionTest, FetchBucketInfoSuccess);
  FRIEND_TEST(S3ObjectActionTest, LoadMetadata);
  FRIEND_TEST(S3ObjectActionTest, SetAuthorizationMeta);
  FRIEND_TEST(S3ObjectActionTest, FetchObjectInfoFailed);
  FRIEND_TEST(S3ObjectActionTest, FetchObjectInfoSuccess);
};

#endif


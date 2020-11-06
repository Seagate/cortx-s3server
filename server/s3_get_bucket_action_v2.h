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

#ifndef __S3_SERVER_S3_GET_BUCKET_ACTION_V2_H__
#define __S3_SERVER_S3_GET_BUCKET_ACTION_V2_H__

#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_kvs_reader.h"
#include "s3_factory.h"
#include "s3_object_list_v2_response.h"
#include "s3_get_bucket_action.h"
#include "s3_common_utilities.h"

class S3GetBucketActionV2 : public S3GetBucketAction {
 protected:
  bool request_fetch_owner;
  std::string request_cont_token;
  std::string request_start_after;
  std::shared_ptr<S3CommonUtilities::S3Obfuscator> obfuscator;

 public:
  S3GetBucketActionV2(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3CommonUtilities::S3Obfuscator> obfuscator = nullptr);
  ~S3GetBucketActionV2();

  void validate_request();
  void after_validate_request();
  void send_response_to_s3_client();

    // For Testing purpose
  FRIEND_TEST(S3GetBucketActionV2Test, Constructor);
  FRIEND_TEST(S3GetBucketActionV2Test, V2ValidateRequest);
  FRIEND_TEST(S3GetBucketActionV2Test,
              after_validate_request_cont_token_non_empty);
  FRIEND_TEST(S3GetBucketActionV2Test, after_validate_request_cont_token_empty);
  FRIEND_TEST(S3GetBucketActionV2Test, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3GetBucketActionV2Test, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3GetBucketActionV2Test, SendResponseToClientSuccess);
  FRIEND_TEST(S3GetBucketActionV2Test, SendResponseToClientInternalError);
};
#endif

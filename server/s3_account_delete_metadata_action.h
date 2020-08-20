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

#ifndef __S3_ACCOUNT_DELETE_METADATA_ACTION_H__
#define __S3_ACCOUNT_DELETE_METADATA_ACTION_H__

#include <tuple>
#include <vector>

#include "s3_action_base.h"
#include "s3_factory.h"
#include "s3_put_bucket_body.h"

class S3AccountDeleteMetadataAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::string account_id_from_uri;
  std::string bucket_account_id_key_prefix;

  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory;

  void validate_request();

  void fetch_first_bucket_metadata();
  void fetch_first_bucket_metadata_successful();
  void fetch_first_bucket_metadata_failed();

 public:
  S3AccountDeleteMetadataAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory = nullptr);

  void setup_steps();
  void send_response_to_s3_client();

  // Google Tests
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, Constructor);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, ValidateRequestSuceess);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, ValidateRequestFailed);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, FetchFirstBucketMetadata);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, FetchFirstBucketMetadataExist);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest,
              FetchFirstBucketMetadataNotExist);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest,
              FetchFirstBucketMetadataMissing);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest,
              FetchFirstBucketMetadataFailed);
};

#endif

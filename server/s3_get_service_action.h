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

#ifndef __S3_SERVER_S3_GET_SERVICE_ACTION_H__
#define __S3_SERVER_S3_GET_SERVICE_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_motr_kvs_reader.h"
#include "s3_service_list_response.h"

class S3GetServiceAction : public S3Action {
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<MotrAPI> s3_motr_api;
  m0_uint128 bucket_list_index_oid;
  std::string last_key;  // last key during each iteration
  std::string key_prefix;  // holds account id
  S3ServiceListResponse bucket_list;
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  bool fetch_successful;
  std::shared_ptr<S3MotrKVSReaderFactory> s3_motr_kvs_reader_factory;

  std::string get_search_bucket_prefix() {
    return request->get_account_id() + "/";
  }

 public:
  S3GetServiceAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory =
          nullptr);
  void setup_steps();
  void initialization();
  void get_next_buckets();
  void get_next_buckets_successful();
  void get_next_buckets_failed();

  void send_response_to_s3_client();
  FRIEND_TEST(S3GetServiceActionTest, ConstructorTest);
  FRIEND_TEST(S3GetServiceActionTest,
              GetNextBucketCallsGetBucketListIndexIfMetadataPresent);
  FRIEND_TEST(S3GetServiceActionTest,
              GetNextBucketDoesNotCallsGetBucketListIndexIfMetadataFailed);
  FRIEND_TEST(S3GetServiceActionTest, GetNextBucketSuccessful);
  FRIEND_TEST(S3GetServiceActionTest,
              GetNextBucketFailedMotrReaderStateMissing);
  FRIEND_TEST(S3GetServiceActionTest,
              GetNextBucketFailedMotrReaderStatePresent);
  FRIEND_TEST(S3GetServiceActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3GetServiceActionTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3GetServiceActionTest, SendResponseToClientSuccess);
};

#endif

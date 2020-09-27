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

#ifndef __S3_SERVER_S3_GET_BUCKET_ACTION_H__
#define __S3_SERVER_S3_GET_BUCKET_ACTION_H__

#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_kvs_reader.h"
#include "s3_factory.h"
#include "s3_object_list_response.h"

class S3GetBucketAction : public S3BucketAction {
  std::shared_ptr<S3MotrKVSReaderFactory> s3_motr_kvs_reader_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<MotrAPI> s3_motr_api;
  size_t max_record_count;
  short retry_count;
  // Identify total keys visited/touched in the object listing
  size_t total_keys_visited;

 protected:
  std::shared_ptr<S3ObjectListResponse> object_list;
  std::string last_key;  // last key during each iteration
  bool fetch_successful;

  // Helpers
  std::string get_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name();
  }

  std::string get_multipart_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name() + "/Multipart";
  }

  // Request Input params
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  size_t max_keys;

 protected:
  // Number of Keys returned in the List Objects V2 response
  size_t key_Count;

 public:
  S3GetBucketAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  virtual ~S3GetBucketAction();
  void setup_steps();
  // Setter method to set the derived object list response (e.g., set Object
  // list V2 response)
  void set_object_list_response(
      const std::shared_ptr<S3ObjectListResponse>& src_obj_list) {
    object_list = src_obj_list;
  }
  // Derived class can override S3 request validation logic
  virtual void validate_request();
  virtual void after_validate_request();
  void fetch_bucket_info_failed();
  void get_next_objects();
  void get_next_objects_successful();
  void get_next_objects_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3GetBucketActionTest, Constructor);
  FRIEND_TEST(S3GetBucketActionTest, ObjectListSetup);
  FRIEND_TEST(S3GetBucketActionTest, FetchBucketInfo);
  FRIEND_TEST(S3GetBucketActionTest, FetchBucketInfoFailedMissing);
  FRIEND_TEST(S3GetBucketActionTest, FetchBucketInfoFailedInternalError);
  // FRIEND_TEST(S3GetBucketActionTest, GetNextObjects);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsWithZeroObjects);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessful);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessfulJsonError);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessfulPrefix);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessfulDelimiter);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessfulPrefixDelimiter);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsFailed);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsFailedNoEntries);
  FRIEND_TEST(S3GetBucketActionTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3GetBucketActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3GetBucketActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3GetBucketActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessfulMultiComponentKey);
  FRIEND_TEST(S3GetBucketActionTest,
              GetNextObjectsSuccessfulPrefixDelimMultiComponentKey);
  FRIEND_TEST(S3GetBucketActionTest, GetNextObjectsSuccessfulDelimiterLastKey);
};

#endif

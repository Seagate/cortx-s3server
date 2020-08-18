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

#ifndef __S3_SERVER_S3_GET_MULTIPART_BUCKET_ACTION_H__
#define __S3_SERVER_S3_GET_MULTIPART_BUCKET_ACTION_H__

#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_motr_kvs_reader.h"
#include "s3_factory.h"
#include "s3_object_list_response.h"

class S3GetMultipartBucketAction : public S3BucketAction {
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3MotrKVSReaderFactory> s3_motr_kvs_reader_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;
  S3ObjectListResponse multipart_object_list;
  std::string last_key;  // last key during each iteration
  size_t return_list_size;

  bool fetch_successful;

  std::string get_multipart_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name() + "/Multipart";
  }

  // Request Input params
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  std::string last_uploadid;
  std::string request_marker_uploadid;
  size_t max_uploads;

 public:
  S3GetMultipartBucketAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  void object_list_setup();
  void setup_steps();
  void fetch_bucket_info_failed();
  void get_next_objects();
  void get_next_objects_successful();
  void get_next_objects_failed();
  void get_key_object();
  void get_key_object_successful();
  void get_key_object_failed();
  void send_response_to_s3_client();

  // For testing purpose
  FRIEND_TEST(S3GetMultipartBucketActionTest, Constructor);
  FRIEND_TEST(S3GetMultipartBucketActionTest, ObjectListSetup);
  FRIEND_TEST(S3GetMultipartBucketActionTest, FetchBucketInfoFailedMissing);
  FRIEND_TEST(S3GetMultipartBucketActionTest,
              FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3GetMultipartBucketActionTest, GetNextObjects);
  FRIEND_TEST(S3GetMultipartBucketActionTest, GetNextObjectsWithZeroObjects);
  FRIEND_TEST(S3GetMultipartBucketActionTest, GetNextObjectsSuccessful);
  FRIEND_TEST(S3GetMultipartBucketActionTest,
              GetNextObjectsSuccessfulJsonError);
  FRIEND_TEST(S3GetMultipartBucketActionTest, GetNextObjectsSuccessfulPrefix);
  FRIEND_TEST(S3GetMultipartBucketActionTest,
              GetNextObjectsSuccessfulDelimiter);
  FRIEND_TEST(S3GetMultipartBucketActionTest,
              GetNextObjectsSuccessfulPrefixDelimiter);
  FRIEND_TEST(S3GetMultipartBucketActionTest, GetNextObjectsFailed);
  FRIEND_TEST(S3GetMultipartBucketActionTest, GetNextObjectsFailedNoEntries);
  FRIEND_TEST(S3GetMultipartBucketActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3GetMultipartBucketActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3GetMultipartBucketActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3GetMultipartBucketActionTest,
              SendResponseToClientInternalError);
};

#endif

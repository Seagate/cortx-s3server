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

#ifndef __S3_SERVER_S3_GET_MULTIPART_PART_ACTION_H__
#define __S3_SERVER_S3_GET_MULTIPART_PART_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_motr_kvs_reader.h"
#include "s3_factory.h"
#include "s3_object_list_response.h"

class S3GetMultipartPartAction : public S3BucketAction {
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata;
  S3ObjectListResponse multipart_part_list;
  std::string last_key;  // last key during each iteration
  size_t return_list_size;
  std::string bucket_name;
  std::string object_name;
  std::string upload_id;
  m0_uint128 multipart_oid;

  bool fetch_successful;
  bool invalid_upload_id;

  // Request Input params
  std::string request_marker_key;
  size_t max_parts;

  std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_metadata_factory;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<S3PartMetadataFactory> part_metadata_factory;

  std::string get_part_index_name() {
    return "BUCKET/" + bucket_name + "/" + object_name + "/" + upload_id;
  }

 public:
  S3GetMultipartPartAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory =
          nullptr);

  void setup_steps();

  void get_next_objects();
  void get_next_objects_successful();
  void get_next_objects_failed();
  void get_key_object();
  void get_key_object_successful();
  void get_key_object_failed();
  void fetch_bucket_info_failed();
  void get_multipart_metadata();
  void send_response_to_s3_client();

  // Google tests
  FRIEND_TEST(S3GetMultipartPartActionTest, ConstructorTest);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetMultiPartMetadataPresentOidPresentTest);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetMultiPartMetadataPresentOIDNullTest);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetMultiPartMetadataMissingTest);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetMultiPartMetadataFailedTest);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetMultiPartMetadataFailedToLaunchTest);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetkeyObjectMetadataPresentUploadMisMatchTest);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetkeyObjectMetadataPresentUploadIDMatchTest);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetkeyObjectMetadataMissing);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetkeyObjectMetadataFailed);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetKeyObjectSuccessfulShutdownSet);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetKeyObjectSuccessfulValueEmpty);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetKeyObjectSuccessfulValueNotEmptyListSizeSameAsMaxAllowed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetKeyObjectSuccessfulValueNotEmptyListSizeNotSameAsMaxAllowed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetKeyObjectSuccessfulValueNotEmptyJsonFailed);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetKeyObjectFailedNoMetadata);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetKeyObjectFailedMetadataFailed);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetNextObjectsMultipartPresent);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetNextObjectsMultipartMissing);
  FRIEND_TEST(S3GetMultipartPartActionTest, GetNextObjectsMultipartStateFailed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulRollBackSet);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulListSizeisMaxAllowed);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulListNotTruncated);
  FRIEND_TEST(S3GetMultipartPartActionTest,
              GetNextObjectsSuccessfulGetMoreObjects);
  FRIEND_TEST(S3GetMultipartPartActionTest, SendInternalErrorResponse);
  FRIEND_TEST(S3GetMultipartPartActionTest, SendNoSuchBucketErrorResponse);
  FRIEND_TEST(S3GetMultipartPartActionTest, SendNoSuchUploadErrorResponse);
  FRIEND_TEST(S3GetMultipartPartActionTest, SendSuccessResponse);
  FRIEND_TEST(S3GetMultipartPartActionTest, SendInternalErrorRetry);
};

#endif

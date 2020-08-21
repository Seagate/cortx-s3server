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

#ifndef __S3_SERVER_S3_ABORT_MULTIPART_ACTION_H__
#define __S3_SERVER_S3_ABORT_MULTIPART_ACTION_H__

#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_motr_writer.h"
#include "s3_probable_delete_record.h"

enum class S3AbortMultipartActionState {
  empty,             // Initial state
  validationFailed,  // Any validations failed for request, including metadata
  probableEntryRecordFailed,
  uploadMetadataDeleted,
  uploadMetadataDeleteFailed,
  partsListIndexDeleted,
  partsListIndexDeleteFailed,
  completed,  // All stages done completely
};

class S3AbortMultipartAction : public S3BucketAction {
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata;
  std::shared_ptr<S3PartMetadata> part_metadata;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::string upload_id;
  std::string bucket_name;
  std::string object_name;
  m0_uint128 multipart_oid;
  m0_uint128 part_index_oid;

  // Probable delete record for object OID to be deleted
  std::string oid_str;  // Key for probable delete rec
  std::unique_ptr<S3ProbableDeleteRecord> probable_delete_rec;

  std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_metadata_factory;
  std::shared_ptr<S3PartMetadataFactory> part_metadata_factory;
  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;

  S3AbortMultipartActionState s3_abort_mp_action_state;

 public:
  S3AbortMultipartAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> s3_motr_api = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory =
          nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

  void setup_steps();
  void fetch_bucket_info_failed();
  void get_multipart_metadata();
  void get_multipart_metadata_status();
  void delete_multipart_metadata();
  void delete_multipart_metadata_successful();
  void delete_multipart_metadata_failed();
  void delete_part_index_with_parts();
  void delete_part_index_with_parts_successful();
  void delete_part_index_with_parts_failed();
  void send_response_to_s3_client();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();

  void startcleanup() override;
  void mark_oid_for_deletion();
  void delete_object();
  void remove_probable_record();

  // Google tests
  FRIEND_TEST(S3AbortMultipartActionTest, ConstructorTest);
  FRIEND_TEST(S3AbortMultipartActionTest, GetMultiPartMetadataTest1);
  FRIEND_TEST(S3AbortMultipartActionTest, GetMultiPartMetadataTest2);
  FRIEND_TEST(S3AbortMultipartActionTest, GetMultiPartMetadataTest3);
  FRIEND_TEST(S3AbortMultipartActionTest, GetMultiPartMetadataTest5);
  FRIEND_TEST(S3AbortMultipartActionTest, GetMultiPartMetadataTest6);
  FRIEND_TEST(S3AbortMultipartActionTest, DeleteMultipartMetadataTest1);
  FRIEND_TEST(S3AbortMultipartActionTest, DeleteMultipartMetadataTest2);
  FRIEND_TEST(S3AbortMultipartActionTest, DeleteMultipartMetadataTest3);
  FRIEND_TEST(S3AbortMultipartActionTest, DeleteMultipartMetadataTest4);
  FRIEND_TEST(S3AbortMultipartActionTest, DeleteMultipartMetadataTest5);
  FRIEND_TEST(S3AbortMultipartActionTest, DeleteMultipartMetadataFailedTest);
  FRIEND_TEST(S3AbortMultipartActionTest,
              DeleteMultipartMetadataFailedToLaunchTest);
  FRIEND_TEST(S3AbortMultipartActionTest,
              DeletePartIndexWithPartsMissingIndexTest);
  FRIEND_TEST(S3AbortMultipartActionTest, DeletePartIndexWithPartsTest);
  FRIEND_TEST(S3AbortMultipartActionTest, DeletePartIndexWithPartsFailed);
  FRIEND_TEST(S3AbortMultipartActionTest,
              DeletePartIndexWithPartsFailedToLaunch);
  FRIEND_TEST(S3AbortMultipartActionTest, Send200SuccessToS3Client);
  FRIEND_TEST(S3AbortMultipartActionTest, Send503InternalErrorToS3Client);
};

#endif

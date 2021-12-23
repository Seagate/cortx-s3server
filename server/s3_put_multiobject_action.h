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

#ifndef __S3_SERVER_S3_PUT_MULTIOBJECT_ACTION_H__
#define __S3_SERVER_S3_PUT_MULTIOBJECT_ACTION_H__

#include <memory>

#include "s3_object_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_writer.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"
#include "s3_probable_delete_record.h"
#include "s3_timer.h"

enum class S3PutPartActionState {
  empty = 0,         // Initial state
  validationFailed,  // Any validations failed for request, including metadata
  probableEntryRecordFailed,
  newObjOidCreated,         // New object created
  newObjOidCreationFailed,  // New object create failed
  writeComplete,            // data write to object completed successfully
  writeFailed,              // data write to object failed
  md5ValidationFailed,      // md5 check failed
  metadataSaved,            // metadata saved for new object
  metadataSaveFailed,       // metadata saved for new object
  completed,                // All stages done completely
};

class S3PutMultiObjectAction : public S3ObjectAction {
  S3PutPartActionState s3_put_action_state;
  struct m0_uint128 old_object_oid;
  int old_layout_id;
  std::string old_pvid_str;
  size_t old_part_size = 0;
  struct m0_uint128 new_object_oid;
  int layout_id;
  S3Timer s3_timer;
  std::shared_ptr<S3PartMetadata> part_metadata = NULL;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata = NULL;
  std::shared_ptr<S3MotrWiter> motr_writer = NULL;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer = nullptr;

  size_t total_data_to_stream;
  S3Timer create_object_timer;
  S3Timer write_content_timer;
  int part_number;
  std::string upload_id;
  unsigned motr_write_payload_size;

  int get_part_number() {
    return atoi((request->get_query_string_value("partNumber")).c_str());
  }

  bool auth_failed;
  bool is_first_write_part_request;
  bool write_failed;
  // These 2 flags help respond to client gracefully when either auth or write
  // fails.
  // Both write and chunk auth happen in parallel.
  bool motr_write_in_progress;
  bool motr_write_completed;  // full object write
  bool auth_in_progress;
  bool auth_completed;  // all chunk auth

  void chunk_auth_successful();
  void chunk_auth_failed();
  void send_chunk_details_if_any();
  void validate_multipart_request();
  void check_part_details();

  std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_metadata_factory;
  std::shared_ptr<S3PartMetadataFactory> part_metadata_factory;
  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> motr_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3AuthClientFactory> auth_factory;

  // Probable delete record for old object OID in case of overwrite
  std::string old_oid_str;  // Key for old probable delete rec
  std::unique_ptr<S3ProbableDeleteRecord> old_probable_del_rec;
  // Probable delete record for new object OID in case of current req failure
  std::string new_oid_str;  // Key for new probable delete rec
  std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;

  // Used only for UT
 protected:
  void _set_layout_id(int layoutid);

 public:
  S3PutMultiObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr);

  void setup_steps();
  // void start();
  void fetch_bucket_info_success();
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void fetch_multipart_metadata();
  void fetch_multipart_failed();
  void fetch_part_info();
  void fetch_part_info_success();
  void fetch_part_info_failed();
  void create_part_object();
  void create_part_object_successful();
  void create_part_object_failed();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();
  // Finally, when object md is saved, we would need S3 BD to
  // process leak due to parallel part PUT requests.
  // Below function adds entry to probable index.
  void add_oid_for_parallel_leak_check();

  void initiate_data_streaming();
  void consume_incoming_content();
  void write_object(std::shared_ptr<S3AsyncBufferOptContainer> buffer);

  void write_object_successful();
  void write_object_failed();

  void save_metadata();
  void save_object_metadata_success();
  void save_metadata_failed();
  void send_response_to_s3_client();
  void set_authorization_meta();

  // Rollback tasks
  void startcleanup() override;
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void remove_old_object_version_metadata();
  void delete_new_object();

  // Google tests
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, ConstructorTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ChunkAuthSucessfulShuttingDown);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, ChunkAuthSucessfulNext);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ChunkAuthFailedWriteSuccessful);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ChunkAuthSucessfulWriteFailed);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, ChunkAuthSucessful);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ChunkAuthSuccessShuttingDown);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ChunkAuthFailedShuttingDown);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, ChunkAuthFailedNext);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, FetchBucketInfoTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              FetchBucketInfoFailedMissingTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              CheckPartNumberFailedInvalidPartTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              FetchBucketInfoFailedInternalErrorTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, FetchMultipartMetadata);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              FetchMultiPartMetadataNoSuchUploadFailed);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              FetchMultiPartMetadataInternalErrorFailed);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, FetchFirstPartInfo);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, SaveMultipartMetadata);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              SaveMultipartMetadataError);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              SaveMultipartMetadataAssert);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              SaveMultipartMetadataFailedInternalError);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              SaveMultipartMetadataFailedServiceUnavailable);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              InitiateDataStreamingForZeroSizeObject);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              InitiateDataStreamingExpectingMoreData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              InitiateDataStreamingWeHaveAllData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ConsumeIncomingShouldWriteIfWeAllData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ConsumeIncomingShouldWriteIfWeExactData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ConsumeIncomingShouldWriteIfWeHaveMoreData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ConsumeIncomingShouldPauseWhenWeHaveTooMuch);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ConsumeIncomingShouldNotWriteWhenWriteInprogress);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              SendChunkDetailsToAuthClientWithSingleChunk);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              SendChunkDetailsToAuthClientWithTwoChunks);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              SendChunkDetailsToAuthClientWithTwoChunksAndOneZeroChunk);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, WriteObject);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              WriteObjectShouldSendChunkDetailsForAuth);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              WriteObjectSuccessfulWhileShuttingDown);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              WriteObjectSuccessfulWhileShuttingDownAndRollback);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              WriteObjectSuccessfulShouldWriteStateAllData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              WriteObjectSuccessfulShouldWriteWhenExactWritableSize);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              WriteObjectSuccessfulDoNextStepWhenAllIsWritten);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              WriteObjectSuccessfulShouldRestartReadingData);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, SaveMetadata);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              WriteObjectSuccessfulShouldSendChunkDetailsForAuth);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, SendErrorResponse);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, SendSuccessResponse);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth, SendFailedResponse);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ValidateObjectKeyLengthNegativeCase);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ValidateMetadataLengthNegativeCase);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ValidateUserMetadataLengthNegativeCase);
  FRIEND_TEST(S3PutMultipartObjectActionTestNoMockAuth,
              ValidateMissingContentLength);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth, CreatePartObject);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              CreatePartObjectFailedTestWhileShutdown);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              CreateObjectFailedTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              CreateObjectFailedToLaunchTest);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              WriteObjectFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3PutMultipartObjectActionTestWithMockAuth,
              SaveObjectMetadataFailed);
};

#endif

/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 17-Mar-2016
 */

#pragma once

#ifndef __S3_SERVER_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_H__
#define __S3_SERVER_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_factory.h"
#include "s3_object_metadata.h"
#include "s3_timer.h"

class S3PutChunkUploadObjectAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ClovisWriter> clovis_writer;
  int layout_id;
  struct m0_uint128 old_object_oid;
  int old_layout_id;
  struct m0_uint128 new_object_oid;
  // Maximum retry count for collision resolution
  unsigned short tried_count;
  // string used for salting the uri
  std::string salt;
  S3Timer create_object_timer;
  S3Timer write_content_timer;

  bool auth_failed;
  bool write_failed;
  // These 2 flags help respond to client gracefully when either auth or write
  // fails.
  // Both write and chunk auth happen in parallel.
  bool clovis_write_in_progress;
  bool clovis_write_completed;  // full object write
  bool auth_in_progress;
  bool auth_completed;  // all chunk auth

  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;
  std::shared_ptr<S3ClovisWriterFactory> clovis_writer_factory;
  std::shared_ptr<ClovisAPI> s3_clovis_api;

  void create_new_oid(struct m0_uint128 current_oid);
  void collision_detected();
  void send_chunk_details_if_any();

  // Used in UT Only
  void _set_layout_id(int layoutid) { layout_id = layoutid; }

 public:
  S3PutChunkUploadObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3ClovisWriterFactory> clovis_s3_factory = nullptr,
      std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr);

  void setup_steps();

  void chunk_auth_successful();
  void chunk_auth_failed();

  void fetch_bucket_info();
  void fetch_object_info();
  void fetch_object_info_status();
  void create_object();
  void create_object_failed();

  void initiate_data_streaming();
  void consume_incoming_content();
  void write_object(std::shared_ptr<S3AsyncBufferOptContainer> buffer);

  void write_object_successful();
  void write_object_failed();
  void save_metadata();
  void delete_old_object_if_present();
  void delete_old_object_failed();
  void send_response_to_s3_client();

  // rollback functions
  void rollback_create();
  void rollback_create_failed();

  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, ConstructorTest);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, FetchBucketInfo);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              FetchObjectInfoReturnedFoundShouldHaveNewOID);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              FetchObjectInfoReturnedNotFoundShouldUseURL2OID);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              FetchObjectInfoReturnedInvalidStateReportsError);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, CreateObjectFirstAttempt);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              CreateObjectSecondAttempt);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              CreateObjectFailedTestWhileShutdown);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              CreateObjectFailedWithCollisionExceededRetry);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              CreateObjectFailedWithCollisionRetry);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, CreateObjectFailedTest);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, RollbackTest);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, CreateNewOidTest);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, RollbackFailedTest1);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, RollbackFailedTest2);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              InitiateDataStreamingForZeroSizeObject);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              InitiateDataStreamingExpectingMoreData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              InitiateDataStreamingWeHaveAllData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              ConsumeIncomingShouldWriteIfWeAllData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              ConsumeIncomingShouldWriteIfWeExactData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              ConsumeIncomingShouldWriteIfWeHaveMoreData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              ConsumeIncomingShouldPauseWhenWeHaveTooMuch);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              ConsumeIncomingShouldNotWriteWhenWriteInprogress);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectShouldWriteContentAndMarkProgress);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectEntityFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectEntityOpenFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulWhileShuttingDown);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulWhileShuttingDownAndRollback);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulShouldWriteStateAllData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulShouldWriteWhenExactWritableSize);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulShouldWriteSomeDataWhenMoreData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulDoNextStepWhenAllIsWritten);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              WriteObjectSuccessfulShouldRestartReadingData);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, SaveMetadata);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, DeleteObjectNotRequired);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              DeleteObjectSinceItsPresent);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, DeleteObjectFailed);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth,
              SendResponseWhenShuttingDown);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, SendErrorResponse);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, SendSuccessResponse);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestNoAuth, SendFailedResponse);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth, ConstructorTest);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              InitiateDataStreamingShouldInitChunkAuth);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              SendChunkDetailsToAuthClientWithSingleChunk);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              SendChunkDetailsToAuthClientWithTwoChunks);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              SendChunkDetailsToAuthClientWithTwoChunksAndOneZeroChunk);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              WriteObjectShouldSendChunkDetailsForAuth);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              WriteObjectSuccessfulShouldSendChunkDetailsForAuth);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthSuccessfulWhileShuttingDown);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthFailedWhileShuttingDown);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthSuccessfulWriteInProgress);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthSuccessfulWriteFailed);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthSuccessfulWriteSuccessful);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthFailedWriteFailed);
  FRIEND_TEST(S3PutChunkUploadObjectActionTestWithAuth,
              ChunkAuthFailedWriteSuccessful);
};

#endif

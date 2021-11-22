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

#ifndef __S3_SERVER_S3_PUT_OBJECT_ACTION_H__
#define __S3_SERVER_S3_PUT_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>
#include "s3_put_object_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_writer.h"
#include "s3_factory.h"
#include "s3_object_metadata.h"
#include "s3_probable_delete_record.h"
#include "s3_timer.h"
#include "evhtp_wrapper.h"

class S3PutObjectAction : public S3ObjectAction {
  S3PutObjectActionState s3_put_action_state;
  struct m0_uint128 old_object_oid;
  int old_layout_id;
  struct m0_uint128 new_object_oid;
  // Maximum retry count for collision resolution
  unsigned short tried_count;
  int layout_id;
  int number_of_parts;
  unsigned motr_write_payload_size;
  // string used for salting the uri
  std::string salt;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;

  size_t total_data_to_stream;
  S3Timer s3_timer;
  bool write_in_progress;

  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3PutTagsBodyFactory> put_object_tag_body_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3ObjectMetadata> new_object_metadata;
  std::map<std::string, std::string> new_object_tags_map;

  // Probable delete record for old object OID in case of overwrite
  std::unique_ptr<S3ProbableDeleteRecord> old_probable_del_rec;
  std::string old_oid_str;  // Key for old probable delete rec
  std::vector<std::unique_ptr<S3ProbableDeleteRecord>> probable_del_rec_list;
  // Probable delete record for new object OID in case of current req failure
  std::string new_oid_str;  // Key for new probable delete rec
  std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;
  std::vector<struct m0_uint128> old_obj_oids;
  std::vector<struct m0_fid> old_obj_pvids;
  std::vector<int> old_obj_layout_ids;

  void create_new_oid(struct m0_uint128 current_oid);
  void collision_detected();

  // Only for use with UT
 protected:
  void _set_layout_id(int layout_id);

 public:
  S3PutObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_factory = nullptr,
      std::shared_ptr<S3PutTagsBodyFactory> put_tags_body_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

  void setup_steps();
  // void start();

  void validate_x_amz_tagging_if_present();
  void parse_x_amz_tagging_header(std::string content);
  void validate_tags();
  void validate_put_request();
  void set_authorization_meta();
  void fetch_bucket_info_failed();
  void fetch_object_info_success();
  void fetch_object_info_failed();
  void create_object();
  void create_object_successful();
  void create_object_failed();
  void fetch_ext_object_info_failed();
  void fetch_ext_object_info_success();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();
  void add_part_object_to_probable_dead_oid_list(
      const std::shared_ptr<S3ObjectMetadata> &,
      std::vector<std::unique_ptr<S3ProbableDeleteRecord>> &);
  // Finally, when object md is saved, we would need S3 BD to
  // process leak due to parallel PUT requests.
  // Below function adds entry to probable index.
  void add_oid_for_parallel_leak_check();

  void initiate_data_streaming();
  void consume_incoming_content();
  void write_object(std::shared_ptr<S3AsyncBufferOptContainer> buffer);

  void write_object_successful();
  void write_object_failed();
  void save_metadata();
  void save_object_metadata_success();
  void save_object_metadata_failed();
  void send_response_to_s3_client();

  // Rollback tasks
  void startcleanup() override;
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void delete_old_object_success();
  void remove_old_object_version_metadata();
  void delete_new_object();

  FRIEND_TEST(S3PutObjectActionTest, ConstructorTest);
  FRIEND_TEST(S3PutObjectActionTest, ValidateRequestTags);
  FRIEND_TEST(S3PutObjectActionTest, VaidateEmptyTags);
  FRIEND_TEST(S3PutObjectActionTest, ValidatePUTContentLengthAs5GB);
  FRIEND_TEST(S3PutObjectActionTest, ValidatePUTContentLengthGreaterThan5GB);
  FRIEND_TEST(S3PutObjectActionTest, VaidateInvalidTagsCase1);
  FRIEND_TEST(S3PutObjectActionTest, VaidateInvalidTagsCase2);
  FRIEND_TEST(S3PutObjectActionTest, VaidateInvalidTagsCase3);
  FRIEND_TEST(S3PutObjectActionTest, VaidateInvalidTagsCase4);
  FRIEND_TEST(S3PutObjectActionTest, VaidateSpecialCharTagsCase1);
  FRIEND_TEST(S3PutObjectActionTest, VaidateSpecialCharTagsCase2);
  FRIEND_TEST(S3PutObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3PutObjectActionTest, FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3PutObjectActionTest, FetchObjectInfoWhenBucketFailedTolaunch);
  FRIEND_TEST(S3PutObjectActionTest, FetchObjectInfoWhenBucketFailed);
  FRIEND_TEST(S3PutObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3PutObjectActionTest,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3PutObjectActionTest,
              FetchObjectInfoReturnedFoundShouldHaveNewOID);
  FRIEND_TEST(S3PutObjectActionTest,
              FetchObjectInfoReturnedNotFoundShouldUseURL2OID);
  FRIEND_TEST(S3PutObjectActionTest,
              FetchObjectInfoReturnedInvalidStateReportsError);
  FRIEND_TEST(S3PutObjectActionTest, CreateObjectFirstAttempt);
  FRIEND_TEST(S3PutObjectActionTest, CreateObjectSecondAttempt);
  FRIEND_TEST(S3PutObjectActionTest, CreateObjectFailedTestWhileShutdown);
  FRIEND_TEST(S3PutObjectActionTest,
              CreateObjectFailedWithCollisionExceededRetry);
  FRIEND_TEST(S3PutObjectActionTest, CreateObjectFailedWithCollisionRetry);
  FRIEND_TEST(S3PutObjectActionTest, CreateObjectFailedTest);
  FRIEND_TEST(S3PutObjectActionTest, CreateObjectFailedToLaunchTest);
  FRIEND_TEST(S3PutObjectActionTest, CreateNewOidTest);
  FRIEND_TEST(S3PutObjectActionTest, InitiateDataStreamingForZeroSizeObject);
  FRIEND_TEST(S3PutObjectActionTest, InitiateDataStreamingExpectingMoreData);
  FRIEND_TEST(S3PutObjectActionTest, InitiateDataStreamingWeHaveAllData);
  FRIEND_TEST(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeAllData);
  FRIEND_TEST(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeExactData);
  FRIEND_TEST(S3PutObjectActionTest,
              ConsumeIncomingShouldWriteIfWeHaveMoreData);
  FRIEND_TEST(S3PutObjectActionTest,
              ConsumeIncomingShouldPauseWhenWeHaveTooMuch);
  FRIEND_TEST(S3PutObjectActionTest,
              ConsumeIncomingShouldNotWriteWhenWriteInprogress);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectShouldWriteContentAndMarkProgress);
  FRIEND_TEST(S3PutObjectActionTest, WriteObjectFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3PutObjectActionTest, WriteObjectFailedDuetoEntityOpenFailure);
  FRIEND_TEST(S3PutObjectActionTest, WriteObjectSuccessfulWhileShuttingDown);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectSuccessfulWhileShuttingDownAndRollback);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectSuccessfulShouldWriteStateAllData);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectSuccessfulShouldWriteWhenExactWritableSize);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectSuccessfulShouldWriteSomeDataWhenMoreData);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectSuccessfulDoNextStepWhenAllIsWritten);
  FRIEND_TEST(S3PutObjectActionTest,
              WriteObjectSuccessfulShouldRestartReadingData);
  FRIEND_TEST(S3PutObjectActionTest, SaveMetadata);
  FRIEND_TEST(S3PutObjectActionTest, SaveObjectMetadataFailed);
  FRIEND_TEST(S3PutObjectActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3PutObjectActionTest, SendErrorResponse);
  FRIEND_TEST(S3PutObjectActionTest, SendSuccessResponse);
  FRIEND_TEST(S3PutObjectActionTest, SendFailedResponse);
  FRIEND_TEST(S3PutObjectActionTest, ConsumeIncomingContentRequestTimeout);
  FRIEND_TEST(S3PutObjectActionTest, DelayedDeleteOldObject);
};

#endif

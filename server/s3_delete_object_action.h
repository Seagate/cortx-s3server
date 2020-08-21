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

#ifndef __S3_SERVER_S3_DELETE_OBJECT_ACTION_H__
#define __S3_SERVER_S3_DELETE_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_object_action_base.h"
#include "s3_motr_writer.h"
#include "s3_factory.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_probable_delete_record.h"

enum class S3DeleteObjectActionState {
  empty,             // Initial state
  validationFailed,  // Any validations failed for request, including missing
                     // metadata
  probableEntryRecordFailed,
  metadataDeleted,
  metadataDeleteFailed,
};

class S3DeleteObjectAction : public S3ObjectAction {
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;

  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;

  // Probable delete record for object OID to be deleted
  std::string oid_str;  // Key for probable delete rec
  std::unique_ptr<S3ProbableDeleteRecord> probable_delete_rec;
  S3DeleteObjectActionState s3_del_obj_action_state;

 public:
  S3DeleteObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> writer_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<MotrAPI> motr_api = nullptr);

  void setup_steps();

  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void delete_metadata();
  void delete_metadata_failed();
  void delete_metadata_successful();
  void set_authorization_meta();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();

  void startcleanup() override;
  void mark_oid_for_deletion();
  void delete_object();
  void remove_probable_record();

  void send_response_to_s3_client();

  FRIEND_TEST(S3DeleteObjectActionTest, ConstructorTest);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchObjectInfo);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketFetchFailed);
  FRIEND_TEST(S3DeleteObjectActionTest,
              DeleteObjectWhenMetadataDeleteFailedToLaunch);
  FRIEND_TEST(S3DeleteObjectActionTest,
              FetchObjectInfoWhenBucketFetchFailedToLaunch);
  FRIEND_TEST(S3DeleteObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3DeleteObjectActionTest,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteMetadataWhenObjectPresent);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteMetadataWhenObjectAbsent);
  FRIEND_TEST(S3DeleteObjectActionTest,
              DeleteMetadataWhenObjectMetadataFetchFailed);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteObjectWhenMetadataDeleteFailed);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteObjectMissingShouldReportDeleted);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteObjectFailedShouldReportDeleted);
  FRIEND_TEST(S3DeleteObjectActionTest, SendErrorResponse);
  FRIEND_TEST(S3DeleteObjectActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3DeleteObjectActionTest, SendSuccessResponse);
};

#endif

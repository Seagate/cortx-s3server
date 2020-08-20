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

#ifndef __S3_SERVER_S3_POST_MULTIPARTOBJECT_ACTION_H__
#define __S3_SERVER_S3_POST_MULTIPARTOBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include "s3_object_action_base.h"
#include "s3_motr_writer.h"
#include "s3_factory.h"
#include "s3_part_metadata.h"
#include "s3_probable_delete_record.h"
#include "s3_timer.h"
#include "s3_uuid.h"
#include "evhtp_wrapper.h"

class S3PostMultipartObjectAction : public S3ObjectAction {
  struct m0_uint128 oid;
  int layout_id;
  struct m0_uint128 old_oid;
  int old_layout_id;
  struct m0_uint128 multipart_index_oid;
  short tried_count;
  std::string salt;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata;
  std::shared_ptr<S3PartMetadata> part_metadata;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::string upload_id;

  S3Timer create_object_timer;

  std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_metadata_factory;
  std::shared_ptr<S3PartMetadataFactory> part_metadata_factory;
  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3PutTagsBodyFactory> put_object_tag_body_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::map<std::string, std::string> new_object_tags_map;

 protected:
  void set_oid(struct m0_uint128 new_oid) { oid = new_oid; }
  struct m0_uint128 get_oid() {
    return oid;
  }

 public:
  S3PostMultipartObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_writer_factory = nullptr,
      std::shared_ptr<S3PutTagsBodyFactory> put_tags_body_factory = nullptr,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

  void setup_steps();

  void validate_x_amz_tagging_if_present();
  void parse_x_amz_tagging_header(std::string content);
  void validate_tags();
  void fetch_bucket_info_failed();
  void check_multipart_object_info_status();
  void check_object_state();
  void fetch_object_info_success();
  void fetch_object_info_failed();
  void check_upload_is_inprogress();
  void create_object();
  void create_object_successful();
  void create_object_failed();
  virtual void collision_occured();
  void create_new_oid(struct m0_uint128 current_oid);
  void save_upload_metadata();
  void save_upload_metadata_failed();
  void create_part_meta_index();
  void create_part_meta_index_successful();
  void create_part_meta_index_failed();
  void send_response_to_s3_client();

  void set_authorization_meta();

  // rollback functions
  void startcleanup() override;
  void rollback_create();
  void rollback_create_failed();
  void rollback_create_part_meta_index();
  void rollback_create_part_meta_index_failed();

  // TODO - clean up empty objects, low risk due to no storage occupied
  // void add_object_oid_to_probable_dead_oid_list();
  // void add_object_oid_to_probable_dead_oid_list_failed();

  std::shared_ptr<S3RequestObject> get_request() { return request; }

  FRIEND_TEST(S3PostMultipartObjectTest, ConstructorTest);
  FRIEND_TEST(S3PostMultipartObjectTest, FetchBucketInfo);
  FRIEND_TEST(S3PostMultipartObjectTest, UploadInProgress);
  FRIEND_TEST(S3PostMultipartObjectTest, UploadInProgressMetadataCorrupt);
  FRIEND_TEST(S3PostMultipartObjectTest, ValidateRequestTags);
  FRIEND_TEST(S3PostMultipartObjectTest, VaidateEmptyTags);
  FRIEND_TEST(S3PostMultipartObjectTest, CreateObject);
  FRIEND_TEST(S3PostMultipartObjectTest, CreateObjectFailed);
  FRIEND_TEST(S3PostMultipartObjectTest, CreateObjectFailedToLaunch);
  FRIEND_TEST(S3PostMultipartObjectTest, CreateObjectFailedDueToCollision);
  FRIEND_TEST(S3PostMultipartObjectTest, CreateNewOid);
  FRIEND_TEST(S3PostMultipartObjectTest, FetchObjectInfoMultipartPresent);
  FRIEND_TEST(S3PostMultipartObjectTest, FetchObjectInfoMultipartNotPresent);
  FRIEND_TEST(S3PostMultipartObjectTest, FetchObjectInfoObjectNotPresent);
  FRIEND_TEST(S3PostMultipartObjectTest, FetchObjectInfoStatusObjectNotPresent);
  FRIEND_TEST(S3PostMultipartObjectTest, FetchObjectInfoStatusObjectPresent);
  FRIEND_TEST(S3PostMultipartObjectTest, CollisionOccured);
  FRIEND_TEST(S3PostMultipartObjectTest, RollbackCreate);
  FRIEND_TEST(S3PostMultipartObjectTest, RollbackCreateFailedMetadataMissing);
  FRIEND_TEST(S3PostMultipartObjectTest, RollbackCreateFailedMetadataFailed);
  FRIEND_TEST(S3PostMultipartObjectTest, RollbackCreateFailedMetadataFailed1);
  FRIEND_TEST(S3PostMultipartObjectTest, RollbackPartMetadataIndex);
  FRIEND_TEST(S3PostMultipartObjectTest, SaveUploadMetadata);
  FRIEND_TEST(S3PostMultipartObjectTest, RollbackPartMetadataIndexFailed);
  FRIEND_TEST(S3PostMultipartObjectTest, SaveUploadMetadataFailed);
  FRIEND_TEST(S3PostMultipartObjectTest, SaveUploadMetadataFailedToLaunch);
  FRIEND_TEST(S3PostMultipartObjectTest, CreatePartMetadataIndex);
  FRIEND_TEST(S3PostMultipartObjectTest, Send500ResponseToClient);
  FRIEND_TEST(S3PostMultipartObjectTest, Send404ResponseToClient);
  FRIEND_TEST(S3PostMultipartObjectTest, Send200ResponseToClient);
};

#endif

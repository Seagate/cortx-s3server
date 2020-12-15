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

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest_prod.h>

#include "lib/types.h"  // struct m0_uint128
#include "s3_object_action_base.h"

enum class S3PutObjectActionState {
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

class MotrAPI;
class S3MotrKVSWriter;
class S3MotrKVSWriterFactory;
class S3MotrWiter;
class S3MotrWriterFactory;
class S3ObjectMetadata;
class S3ProbableDeleteRecord;

class S3PutObjectActionBase : public S3ObjectAction {
 protected:
  S3PutObjectActionBase(
      std::shared_ptr<S3RequestObject> s3_request,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

  void _set_layout_id(int layout_id);  // Only for use with UT

  // Destination bucket
  void fetch_bucket_info_failed() final;

  // Destination object
  void fetch_object_info_success() final;
  void fetch_object_info_failed() final;

  // Create object
  void create_object();
  void create_object_successful();
  void create_object_failed();
  void collision_detected();
  void create_new_oid(struct m0_uint128);

  void set_authorization_meta();

  // Rollback tasks
  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();
  void startcleanup() final;
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void remove_old_object_version_metadata();
  void delete_new_object();

  // Data
  struct m0_uint128 old_object_oid = {};
  struct m0_uint128 new_object_oid = {};

  std::string old_oid_str;  // Key for old probable delete rec
  std::string new_oid_str;  // Key for new probable delete rec

  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<MotrAPI> s3_motr_api;

  std::shared_ptr<S3ObjectMetadata> new_object_metadata;

  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;

  // Probable delete record for old object OID in case of overwrite
  std::unique_ptr<S3ProbableDeleteRecord> old_probable_del_rec;
  // Probable delete record for new object OID in case of current req failure
  std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;

  size_t total_data_to_stream = 0;
  size_t motr_unit_size = 0;

  int layout_id = -1;
  int old_layout_id = -1;
  unsigned motr_write_payload_size = 0;
  unsigned short tried_count = 0;

  S3PutObjectActionState s3_put_action_state = S3PutObjectActionState::empty;
  bool write_in_progress = false;
};

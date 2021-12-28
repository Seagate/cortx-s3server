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
#include "s3_common_utilities.h"

enum class S3PutObjectActionState {
  empty = 0,         // Initial state
  validationFailed,  // Any validations failed for request, including metadata
  probableEntryRecordFailed,
  newObjOidCreated,         // New object created
  newObjOidCreationFailed,  // New object create failed
  writeComplete,            // data write to object completed successfully
  writeFailed,              // data write to object failed
  md5ValidationFailed,      // md5 check failed
  saveDataUsageSuccess,     // Data Usage saving
  saveDataUsageFailed,      // Data Usage saving
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

  // On successful load of extends/parts of old object
  void fetch_old_ext_object_info_success();

  // additional bucket (Incase of copy-object API)
  void fetch_additional_bucket_info_failed() final;
  void fetch_additional_object_info_failed() final;

  // Fetching of extended metadata failed
  void fetch_ext_object_info_failed();

  // Create object
  void create_object();
  void create_object_successful();
  void create_object_failed();
  void create_objects();
  void create_parts_fragments(int index);
  void create_part_fragment_successful();
  void create_part_fragment_failed();
  void collision_detected();
  void create_new_oid(struct m0_uint128);

  void set_authorization_meta();

  // Rollback tasks
  // Pass 'only_old_object' as True, when we want only old/existing object (or
  // its parts) entries to be added to probable delete index
  void add_object_oid_to_probable_dead_oid_list(bool only_old_object = false);
  void add_object_oid_to_probable_dead_oid_list_failed();
  void add_part_object_to_probable_dead_oid_list(
      const std::shared_ptr<S3ObjectMetadata> &,
      std::vector<std::unique_ptr<S3ProbableDeleteRecord>> &);
  void add_parts_fragment_oid_to_probable_dead_oid_list(
      std::shared_ptr<S3ObjectMetadata> &, struct s3_part_frag_context);
  void add_parts_fragment_oid_to_probable_dead_oid_list_success();

  void startcleanup() final;
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void delete_old_object_success();
  void remove_old_object_version_metadata();
  void delete_new_object();

  // Data
  struct m0_uint128 old_object_oid = {};
  struct m0_uint128 new_object_oid = {};
  struct m0_uint128 new_ext_oid = {};

  std::string old_oid_str;  // Key for old probable delete rec
  std::string new_oid_str;  // Key for new probable delete rec
  std::string new_ext_oid_str;
  std::vector<std::string> new_oid_str_list;

  std::shared_ptr<S3MotrWiter> motr_writer;
  std::vector<std::shared_ptr<S3MotrWiter>> motr_writer_list;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3ObjectExtendedMetadata> extended_obj = nullptr;
  std::vector<struct s3_part_frag_context> part_fragment_context_list;
  std::vector<struct s3_part_frag_context> new_part_frag_ctx_list;

  std::shared_ptr<S3ObjectMetadata> new_object_metadata;

  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;

  // Probable delete record for old object OID in case of overwrite
  std::unique_ptr<S3ProbableDeleteRecord> old_probable_del_rec;
  std::vector<std::unique_ptr<S3ProbableDeleteRecord>> probable_del_rec_list;
  // Probable delete record for new object OID in case of current req failure
  std::vector<std::unique_ptr<S3ProbableDeleteRecord>>
      new_probable_del_rec_list;
  std::vector<struct m0_uint128> old_obj_oids;
  std::vector<struct m0_fid> old_obj_pvids;
  std::vector<int> old_obj_layout_ids;

  size_t total_data_to_stream = 0;
  size_t motr_unit_size = 0;
  size_t max_part_size = 0;
  int total_objects = 0;
  bool is_multipart = false;

  int layout_id = -1;
  int old_layout_id = -1;
  unsigned motr_write_payload_size = 0;
  unsigned short tried_count = 0;
  int index = 0;

  S3PutObjectActionState s3_put_action_state = S3PutObjectActionState::empty;
  bool write_in_progress = false;
};

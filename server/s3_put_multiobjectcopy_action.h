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

#ifndef __S3_SERVER_S3_PUT_MULTIOBJECTCOPY_ACTION_H__
#define __S3_SERVER_S3_PUT_MULTIOBJECTCOPY_ACTION_H__

#include <memory>

#include "s3_object_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_writer.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"
#include "s3_probable_delete_record.h"
#include "s3_object_data_copier.h"
#include "s3_put_object_action_base.h"

class S3PutMultiObjectCopyAction : public S3PutObjectActionBase {

  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::unique_ptr<S3ObjectDataCopier> object_data_copier;
  size_t total_data_to_stream;
  struct m0_uint128 old_object_oid;
  int old_layout_id;
  struct m0_uint128 new_object_oid;
  int layout_id;
  std::shared_ptr<S3PartMetadata> part_metadata = NULL;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata = NULL;
  std::shared_ptr<S3MotrWiter> motr_writer = NULL;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer = nullptr;
  std::string auth_acl;
  int max_parallel_copy = 0;
  bool response_started = false;

  int part_number;
  std::string upload_id;

  int get_part_number() {
    return atoi((request->get_query_string_value("partNumber")).c_str());
  }

  bool auth_failed;
  bool is_first_write_part_request;
  bool auth_in_progress;
  bool auth_completed;  // all chunk auth

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
  S3PutMultiObjectCopyAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr);

  void setup_steps();
  std::string get_response_xml();
  // void start();
  void set_authorization_meta();
  void set_source_bucket_authorization_metadata();
  void check_source_bucket_authorization();
  void check_source_bucket_authorization_success();
  void check_source_bucket_authorization_failed();
  void check_destination_bucket_authorization_failed();
  void fetch_multipart_metadata();
  void fetch_multipart_failed();
  void fetch_part_info();
  void fetch_part_info_success();
  void fetch_part_info_failed();
  void create_part_object();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();

  void initiate_copy_object();
  void copy_object();
  bool copy_object_cb();
  void copy_object_success();
  void copy_object_failed();

  void save_metadata();
  void save_object_metadata_success();
  void save_metadata_failed();
  void start_response();
  bool if_source_and_destination_same();
  void send_response_to_s3_client();

  // Rollback tasks
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void remove_old_object_version_metadata();
  void delete_new_object();
};

#endif

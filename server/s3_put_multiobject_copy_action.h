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

#ifndef __S3_SERVER_S3_PUT_MULTIPART_COPY_ACTION_H__
#define __S3_SERVER_S3_PUT_MULTIPART_COPY_ACTION_H__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest_prod.h>

#include "s3_put_object_action_base.h"
#include "s3_object_data_copier.h"
#include "s3_object_metadata.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_writer.h"
#include "s3_part_metadata.h"
#include "s3_probable_delete_record.h"
#include "s3_timer.h"

const uint64_t MaxPartCopySourcePartSize = 5368709120UL;  // 5GB
const uint64_t MinRangeCopyObjectSize = 5242880UL;        // 5MB

const char InvalidRequestSourcePartSizeGreaterThan5GB[] =
    "The specified part in copy source is larger than the maximum allowable "
    "size for a part in copy source: 5368709120";
const char InvalidRequestPartCopySourceAndDestinationSame[] =
    "This upload part copy request is illegal because it is trying to copy "
    "an object to itself without changing the object's metadata, storage "
    "class, website redirect location or encryption attributes.";

class S3ObjectDataCopier;

class S3PutMultipartCopyAction : public S3PutObjectActionBase {
  std::string auth_acl;
  S3PutObjectActionState s3_copy_part_action_state;
  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::unique_ptr<S3ObjectDataCopier> object_data_copier;
  std::shared_ptr<S3ObjectDataCopier> fragment_data_copier;
  std::vector<std::shared_ptr<S3ObjectDataCopier> > fragment_data_copier_list;
  std::shared_ptr<S3PartMetadata> part_metadata = NULL;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata = NULL;
  std::shared_ptr<S3MotrKVSWriterFactory> motr_kv_writer_factory;
  std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;

  std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_metadata_factory;
  std::shared_ptr<S3PartMetadataFactory> part_metadata_factory;

  bool response_started = false;
  bool auth_in_progress = false;
  bool auth_failed = false;
  bool auth_completed = false;

  bool motr_write_in_progress = false;
  bool motr_write_completed = false;  // full object write
  bool write_failed = false;

  int part_number;
  std::string upload_id;
  size_t total_data_to_copy = 0;
  size_t source_object_size = 0;

  size_t first_byte_offset_to_copy = 0;
  size_t last_byte_offset_to_copy = 0;
  unsigned motr_write_payload_size;
  bool if_source_and_destination_same();
  S3Timer s3_timer;

  int get_part_number() {
    return strtol((request->get_query_string_value("partNumber")).c_str(), 0,
                  10);
  }

  void set_authorization_meta();
  void check_source_bucket_authorization();
  void check_source_bucket_authorization_success();
  void check_source_bucket_authorization_failed();
  void set_source_bucket_authorization_metadata();
  void check_destination_bucket_authorization_failed();

 public:
  S3PutMultipartCopyAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory = nullptr,
      std::shared_ptr<S3MotrReaderFactory> motr_s3_reader_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr);

  void setup_steps();

  std::string get_response_xml();
  void check_part_details();
  void fetch_multipart_metadata();
  void fetch_multipart_failed();
  bool validate_range_header(const std::string &range_value);
  void validate_multipart_partcopy_request();
  void fetch_part_info();
  void fetch_part_info_success();
  void fetch_part_info_failed();
  void create_part_object();
  void create_objects();
  void create_part();
  void create_part_object_successful();
  void create_part_object_failed();
  void initiate_part_copy();
  void copy_part_object();
  bool copy_part_object_cb();
  void copy_part_object_success();
  void copy_part_object_failed();
  void save_metadata();
  void save_metadata_failed();
  void save_object_metadata_success();
  void save_object_metadata_failed();
  void send_response_to_s3_client();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();

  // Rollback tasks
  void startcleanup();
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void remove_old_object_version_metadata();
  void delete_new_object();
};
#endif  // __S3_SERVER_S3_PUT_MULTIPART_COPY_ACTION_H__

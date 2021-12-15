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

#ifndef __S3_SERVER_S3_POST_COMPLETE_ACTION_H__
#define __S3_SERVER_S3_POST_COMPLETE_ACTION_H__

#include <memory>

#include "s3_object_action_base.h"
#include "s3_motr_writer.h"
#include "s3_factory.h"
#include "s3_part_metadata.h"
#include "s3_probable_delete_record.h"
#include "s3_aws_etag.h"
#include "s3_uuid.h"

enum class S3PostCompleteActionState {
  empty,                         // Initial state
  validationFailed,              // Any validations failed for request,
                                 // incl failed to load metadata for validation
  probableEntryRecordFailed,     // Failed to add entry to probable delete index
  probableEntryRecordSaved,      // Added entry to probable delete index
  abortedSinceValidationFailed,  // multipart(mp) aborted due to different part
                                 // validation failures, mp entry is deleted.
  metadataSaved,                 // metadata saved for new object
  metadataSaveFailed,            // metadata save failed for new object
  completed,                     // All stages done completely
};

class S3PostCompleteAction : public S3ObjectAction {
  S3PostCompleteActionState s3_post_complete_action_state;
  std::shared_ptr<S3MotrKVSReaderFactory> s3_motr_kvs_reader_factory;
  std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_metadata_factory;
  std::shared_ptr<S3PartMetadataFactory> part_metadata_factory;
  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
  std::shared_ptr<S3ObjectMetadata> multipart_metadata;
  std::shared_ptr<S3PartMetadata> part_metadata;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<S3ObjectMetadata> new_object_metadata;
  std::string upload_id;
  std::string bucket_name;
  std::string object_name;
  std::string total_parts;
  std::string etag;
  std::map<std::string, std::string, S3NumStrComparator> parts;
  uint64_t object_size;
  struct s3_motr_idx_layout multipart_index_layout;
  bool delete_multipart_object;
  bool obj_metadata_updated;
  bool response_started;
  void parse_xml_str(std::string &xml_str);
  size_t count_we_requested;
  size_t current_parts_size;
  size_t prev_fetched_parts_size;
  size_t validated_parts_count;
  std::string last_key;
  S3AwsEtag awsetag;
  struct m0_uint128 old_object_oid;
  int old_layout_id;
  struct m0_uint128 new_object_oid;
  struct m0_fid new_pvid;
  int layout_id;

  // Probable delete record for old object OID in case of overwrite
  std::string old_oid_str;  // Key for old probable delete rec
  // Probable delete record for new object OID in case of current req failure
  std::string new_oid_str;  // Key for new probable delete rec

  // List of part object oids belong to new object
  std::vector<struct m0_uint128> new_obj_oids;
  std::vector<struct m0_fid> new_obj_pvids;
  std::vector<int> new_obj_layout_ids;
  // List of part object oids belong to old object
  std::vector<struct m0_uint128> old_obj_oids;
  std::vector<struct m0_fid> old_obj_pvids;
  std::vector<int> old_obj_layout_ids;
  void start_response();
  // Probable delete record for object parts
  std::vector<std::unique_ptr<S3ProbableDeleteRecord>>
      new_parts_probable_del_rec_list;
  std::vector<std::unique_ptr<S3ProbableDeleteRecord>>
      old_parts_probable_del_rec_list;
  std::map<unsigned int, std::string> part_etags;
  std::string generate_etag();
  bool mp_completion_send_space_chk_shutdown();
  // Following state used during partial put kv
  // - while adding probbale leak entries for parts
  unsigned int total_processed_count = 0;
  unsigned int total_keys = 0;
  std::map<std::string, std::string> kv_list;

 public:
  S3PostCompleteAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory =
          nullptr,
      std::shared_ptr<S3PartMetadataFactory> part_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

  void setup_steps();
  std::string get_response_xml();

  void load_and_validate_request();
  void consume_incoming_content();
  bool validate_request_body(std::string &xml_str);
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void fetch_multipart_info();
  void fetch_multipart_info_success();
  void fetch_multipart_info_failed();
  void fetch_ext_object_info_failed();

  void get_next_parts_info();
  void get_next_parts_info_successful();
  void get_next_parts_info_failed();
  bool validate_parts();
  void get_parts_failed();
  void get_part_info(int part);
  void save_metadata();
  void save_object_metadata_succesful();
  void save_object_metadata_failed();
  void delete_multipart_metadata();
  void delete_multipart_metadata_success();
  void delete_multipart_metadata_failed();
  void delete_part_list_index();
  void delete_part_list_index_failed();
  void set_abort_multipart(bool abortit);
  bool is_abort_multipart();
  void send_response_to_s3_client();
  void set_authorization_meta();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_success();
  void add_object_oid_to_probable_dead_oid_list_failed();
  void add_oid_for_parallel_leak_check();

  // Add object part to the new object's extended metadata portion
  void add_part_object_to_object_extended(
      const std::shared_ptr<S3PartMetadata> &part_metadata,
      std::shared_ptr<S3ObjectMetadata> &object_metadata);
  void add_part_object_to_probable_dead_oid_list(
      const std::shared_ptr<S3ObjectMetadata> &,
      std::vector<std::unique_ptr<S3ProbableDeleteRecord>> &,
      bool is_old_object = false);
  void save_probable_list_metadata(std::map<std::string, std::string> &);
  void save_partial_probable_list_metadata(
      std::map<std::string, std::string> &);
  void save_partial_probable_list_metadata_successful(unsigned int);
  void save_partial_probable_list_metadata_failed(unsigned int);

  void startcleanup() override;
  void mark_old_oid_for_deletion();
  void delete_old_object();
  void delete_old_object_success();
  void remove_old_object_version_metadata();
  void remove_old_oid_probable_record();
  // Delete old entries corresponding to old object parts
  // from the extended md index
  void remove_old_fragments();
  void remove_old_ext_metadata_successful();

  void mark_new_oid_for_deletion();
  void delete_new_object();
  void delete_new_object_success();
  // During failure due to md failure, delete fragments,
  // if existed any, from the extended index
  void remove_new_fragments();
  void remove_new_ext_metadata_successful();
  void remove_new_oid_probable_record();

  std::string get_part_index_name() {
    return "BUCKET/" + bucket_name + "/" + object_name + "/" + upload_id;
  }

  std::string get_multipart_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name() + "/Multipart";
  }

  // For Testing purpose
  FRIEND_TEST(S3PostCompleteActionTest, Constructor);
  FRIEND_TEST(S3PostCompleteActionTest, LoadValidateRequestMoreContent);
  FRIEND_TEST(S3PostCompleteActionTest, LoadValidateRequestMalformed);
  FRIEND_TEST(S3PostCompleteActionTest, LoadValidateRequestNoContent);
  FRIEND_TEST(S3PostCompleteActionTest, LoadValidateRequest);
  FRIEND_TEST(S3PostCompleteActionTest, ConsumeIncomingContentMoreContent);
  FRIEND_TEST(S3PostCompleteActionTest, ValidateRequestBody);
  FRIEND_TEST(S3PostCompleteActionTest, ValidateRequestBodyNoParts);
  FRIEND_TEST(S3PostCompleteActionTest, ValidateRequestBodyEmpty);
  FRIEND_TEST(S3PostCompleteActionTest, ValidateRequestBodyOutOrderParts);
  FRIEND_TEST(S3PostCompleteActionTest, ValidateRequestBodyMissingTag);
  FRIEND_TEST(S3PostCompleteActionTest, FetchBucketInfoFailedMissing);
  FRIEND_TEST(S3PostCompleteActionTest,
              FetchBucketInfoFailedServiceUnavailable);
  FRIEND_TEST(S3PostCompleteActionTest, FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3PostCompleteActionTest, FetchObjectInfoFailedMissing);
  FRIEND_TEST(S3PostCompleteActionTest,
              FetchObjectInfoFailedServiceUnavailable);
  FRIEND_TEST(S3PostCompleteActionTest, FetchObjectInfoFailedInternalError);
  FRIEND_TEST(S3PostCompleteActionTest, FetchExtendedObjectInfoFailedMissing);
  FRIEND_TEST(S3PostCompleteActionTest,
              FetchExtendedObjectInfoFailedServiceUnavailable);
  FRIEND_TEST(S3PostCompleteActionTest,
              FetchExtendedObjectInfoFailedInternalError);
  FRIEND_TEST(S3PostCompleteActionTest, FetchMultipartInfo);
  FRIEND_TEST(S3PostCompleteActionTest, FetchMultipartInfoFailedInvalidObject);
  FRIEND_TEST(S3PostCompleteActionTest, FetchMultipartInfoFailedInternalError);
  FRIEND_TEST(S3PostCompleteActionTest, AddPartObjectToDeadOidList);
  FRIEND_TEST(S3PostCompleteActionTest, GetNextPartsInfo);
  FRIEND_TEST(S3PostCompleteActionTest, GetPartsInfoFailed);
  FRIEND_TEST(S3PostCompleteActionTest, GetPartsSuccessful);
  FRIEND_TEST(S3PostCompleteActionTest, GetNextPartsSuccessful);
  FRIEND_TEST(S3PostCompleteActionTest, GetNextPartsSuccessfulNext);
  FRIEND_TEST(S3PostCompleteActionTest, GetNextPartsSuccessfulAbortSet);
  FRIEND_TEST(S3PostCompleteActionTest, GetPartsSuccessfulEntityTooSmall);
  FRIEND_TEST(S3PostCompleteActionTest, GetPartsSuccessfulEntityTooLarge);
  FRIEND_TEST(S3PostCompleteActionTest, GetPartsSuccessfulJsonError);
  FRIEND_TEST(S3PostCompleteActionTest, GetPartsSuccessfulAbortMultiPart);
  FRIEND_TEST(S3PostCompleteActionTest, DeletePartIndex);
  FRIEND_TEST(S3PostCompleteActionTest, DeleteMultipartMetadata);
  FRIEND_TEST(S3PostCompleteActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3PostCompleteActionTest, SendResponseToClientErrorSet);
  FRIEND_TEST(S3PostCompleteActionTest, SendResponseToClientAbortMultipart);
  FRIEND_TEST(S3PostCompleteActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PostCompleteActionTest, DeleteNewObject);
  FRIEND_TEST(S3PostCompleteActionTest, DeleteOldObject);
  FRIEND_TEST(S3PostCompleteActionTest, RemoveOldExtMetadataSuccess);
  FRIEND_TEST(S3PostCompleteActionTest, RemoveNewExtMetadataSuccess);
  FRIEND_TEST(S3PostCompleteActionTest, DeleteOldObjectSuccess);
  FRIEND_TEST(S3PostCompleteActionTest, DelayedDeleteOldObject);
  FRIEND_TEST(S3PostCompleteActionTest, DeleteMultipartMetadataSucessWithAbort);
  FRIEND_TEST(S3PostCompleteActionTest, StartCleanupValidationFailed);
  FRIEND_TEST(S3PostCompleteActionTest, StartCleanupProbableEntryRecordFailed);
  FRIEND_TEST(S3PostCompleteActionTest,
              StartCleanupAbortedSinceValidationFailed);
};

#endif

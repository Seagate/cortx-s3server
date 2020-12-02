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

#ifndef __S3_SERVER_S3_COPY_OBJECT_ACTION_H__
#define __S3_SERVER_S3_COPY_OBJECT_ACTION_H__

#include "s3_object_action_base.h"
#include "s3_object_metadata.h"

enum class S3CopyObjectActionState {
  empty,             // Initial state
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

const u_long MaxCopyObjectSourceSize = 5368709120UL;  // 5GB

const char InvalidRequestSourceObjectSizeGreaterThan5GB[] =
    "The specified copy source is larger than the maximum allowable size for a "
    "copy source: 5368709120";
const char InvalidRequestSourceAndDestinationSame[] =
    "This copy request is illegal because it is trying to copy an object to "
    "itself without changing the object's metadata, storage class, website "
    "redirect location or encryption";

class S3CopyObjectAction : public S3ObjectAction {
  S3CopyObjectActionState s3_copy_action_state;
  bool write_in_progress;
  bool read_in_progress;
  struct m0_uint128 old_object_oid;
  struct m0_uint128 new_object_oid;
  // Maximum retry count for collision resolution
  unsigned short tried_count;
  int layout_id;
  int old_layout_id;
  int motr_unit_size;
  unsigned motr_write_payload_size;
  bool copy_failed = false;

  size_t content_length;
  size_t bytes_left_to_read;
  // string used for salting the uri
  std::string salt;

  std::string source_bucket_name;
  std::string source_object_name;

  std::shared_ptr<S3MotrReader> motr_reader;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;

  std::shared_ptr<S3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3ObjectMetadata> new_object_metadata;
  std::shared_ptr<S3ObjectMetadata> source_object_metadata;
  std::shared_ptr<S3BucketMetadata> source_bucket_metadata;

  std::string new_oid_str;  // Key for new probable delete rec

  // TODO Edit the read_object() and initiate_data_streaming()
  // signatures accordingly in subsequent Check-ins, when the design evolves.
  void read_object();
  void initiate_data_streaming();
  void get_source_bucket_and_object();
  void fetch_source_bucket_info();
  void fetch_source_bucket_info_success();
  void fetch_source_bucket_info_failed();

  void fetch_source_object_info();
  void fetch_source_object_info_success();
  void fetch_source_object_info_failed();

  bool if_source_and_destination_same();

  // Only for use with UT
 protected:
  void _set_layout_id(int layout_id);

 public:
  S3CopyObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motrwriter_s3_factory = nullptr,
      std::shared_ptr<S3MotrReaderFactory> motrreader_s3_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

 private:
  void setup_steps();

  void fetch_bucket_info_failed();   // destination bucket
  void fetch_object_info_failed();   // destination object
  void fetch_object_info_success();  // destination object

  std::string get_response_xml();
  void validate_copyobject_request();
  void create_object();
  void create_object_successful();
  void create_object_failed();
  void copy_object();
  void save_metadata();
  void save_object_metadata_success();
  void save_object_metadata_failed();
  void set_authorization_meta();
  void send_response_to_s3_client();
  void read_data_block();
  void read_data_block_success();
  void read_data_block_failed();
  void write_data_block();
  void write_data_block_success();
  void write_data_block_failed();

  FRIEND_TEST(S3CopyObjectActionTest, GetSourceBucketAndObjectSuccess);
  FRIEND_TEST(S3CopyObjectActionTest, GetSourceBucketAndObjectFailure);
  FRIEND_TEST(S3CopyObjectActionTest, FetchSourceBucketInfo);
  FRIEND_TEST(S3CopyObjectActionTest, FetchSourceBucketInfoFailedMissing);
  FRIEND_TEST(S3CopyObjectActionTest,
              FetchSourceBucketInfoFailedFailedToLaunch);
  FRIEND_TEST(S3CopyObjectActionTest, FetchSourceBucketInfoFailedInternalError);
  FRIEND_TEST(S3CopyObjectActionTest, FetchSourceObjectInfoListIndexNull);
  FRIEND_TEST(S3CopyObjectActionTest, FetchSourceObjectInfoListIndexGood);
  FRIEND_TEST(S3CopyObjectActionTest,
              FetchSourceObjectInfoSuccessGreaterThan5GB);
  FRIEND_TEST(S3CopyObjectActionTest, FetchSourceObjectInfoSuccessLessThan5GB);
  FRIEND_TEST(S3CopyObjectActionTest,
              ValidateCopyObjectRequestSourceBucketEmpty);
  FRIEND_TEST(S3CopyObjectActionTest,
              ValidateCopyObjectRequestSourceAndDestinationSame);
  FRIEND_TEST(S3CopyObjectActionTest, ValidateCopyObjectRequestSuccess);
  FRIEND_TEST(S3CopyObjectActionTest,
              FetchDestinationBucketInfoFailedMetadataMissing);
  FRIEND_TEST(S3CopyObjectActionTest,
              FetchDestinationBucketInfoFailedFailedToLaunch);
  FRIEND_TEST(S3CopyObjectActionTest,
              FetchDestinationBucketInfoFailedInternalError);
  FRIEND_TEST(S3CopyObjectActionTest, FetchDestinationObjectInfoFailed);
  FRIEND_TEST(S3CopyObjectActionTest, FetchDestinationObjectInfoSuccess);
};
#endif  // __S3_SERVER_S3_COPY_OBJECT_ACTION_H__

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

#ifndef __S3_SERVER_S3_MULTIOBJECT_COPY_ACTION_H__
#define __S3_SERVER_S3_MULTIOBJECT_COPY_ACTION_H__

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

enum class S3CopyPartActionState {
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

const uint64_t MaxCopyObjectSourceSize = 5368709120UL;  // 5GB

const char InvalidRequestSourceObjectSizeGreaterThan5GB[] =
    "The specified copy source is larger than the maximum allowable size for a "
    "copy source: 5368709120";
const char InvalidRequestSourceAndDestinationSame[] =
    "This copy request is illegal because it is trying to copy an object to "
    "itself without changing the object's metadata, storage class, website "
    "redirect location or encryption attributes.";

const char DirectiveValueReplace[] = "REPLACE";
const char DirectiveValueCOPY[] = "COPY";

class S3ObjectDataCopier;

class S3MultiObjectCopyAction : public S3PutObjectActionBase {
  std::string auth_acl;
  S3CopyPartActionState s3_copy_part_action_state;
  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::unique_ptr<S3ObjectDataCopier> object_data_copier;
  std::shared_ptr<S3ObjectDataCopier> fragment_data_copier;
  std::vector<std::shared_ptr<S3ObjectDataCopier>> fragment_data_copier_list;

  bool response_started = false;
  int total_parts_fragment_to_be_copied = 0;
  int parts_fragment_copied_or_failed = 0;
  int max_parallel_copy = 0;
  int parts_frg_copy_in_flight_copied_or_failed = 0;
  int parts_frg_copy_in_flight = 0;

  bool if_source_and_destination_same();

  void set_authorization_meta();
  void check_source_bucket_authorization();
  void check_source_bucket_authorization_success();
  void check_source_bucket_authorization_failed();
  void set_source_bucket_authorization_metadata();
  void check_destination_bucket_authorization_failed();
  void copy_each_part_fragment(int index);

 public:
  S3MultiObjectCopyAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MotrAPI> motr_api = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3MotrWriterFactory> motrwriter_s3_factory = nullptr,
      std::shared_ptr<S3MotrReaderFactory> motrreader_s3_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr);

 private:
  void setup_steps();

  std::string get_response_xml();
  void check_part_details();
  void fetch_multipart_metadata();
  void fetch_multipart_failed();
  void validate_copyobject_request();
  void fetch_part_info();
  void fetch_part_info_success();
  void fetch_part_info_failed();
  void create_one_or_more_objects();
  void create_part_object();
  void create_part_object_successful();
  void create_part_object_failed();
  void copy_one_or_more_objects();
  void copy_object();
  bool copy_object_cb();
  void copy_object_success();
  void copy_object_failed();
  void copy_fragments();
  void copy_part_fragment_success(int index);
  void copy_part_fragment_failed(int index);
  void save_fragment_metadata();
  void save_fragment_metadata_successful();
  void save_fragment_metadata_failed();
  void save_metadata();
  void save_object_metadata_success();
  void save_object_metadata_failed();
  void start_response();
  void send_response_to_s3_client();

  void add_object_oid_to_probable_dead_oid_list();
  void add_object_oid_to_probable_dead_oid_list_failed();

  // Rollback tasks
  void startcleanup() override;
  void mark_new_oid_for_deletion();
  void mark_old_oid_for_deletion();
  void remove_old_oid_probable_record();
  void remove_new_oid_probable_record();
  void delete_old_object();
  void remove_old_object_version_metadata();
  void delete_new_object();

  friend class S3MultiObjectCopyActionTest;

  FRIEND_TEST(S3MultiObjectCopyActionTest, GetSourceBucketAndObjectSuccess);
  FRIEND_TEST(S3MultiObjectCopyActionTest, GetSourceBucketAndSpecialObjectSuccess);
  FRIEND_TEST(S3MultiObjectCopyActionTest, UIGetSourceBucketAndObjectSuccess);
  FRIEND_TEST(S3MultiObjectCopyActionTest, GetSourceBucketAndObjectFailure);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchSourceBucketInfoFailedMissing);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              FetchSourceBucketInfoFailedFailedToLaunch);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchSourceBucketInfoFailedInternalError);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchSourceObjectInfoListIndexNull);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchSourceObjectInfoListIndexGood);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              FetchSourceObjectInfoSuccessGreaterThan5GB);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchSourceObjectInfoSuccessLessThan5GB);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              ValidateCopyObjectRequestSourceBucketEmpty);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              ValidateCopyObjectRequestMetadataDirectiveReplace);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              ValidateCopyObjectRequestMetadataDirectiveInvalid);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              ValidateCopyObjectRequestTaggingDirectiveReplace);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              ValidateCopyObjectRequestTaggingDirectiveInvalid);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              ValidateCopyObjectRequestSourceAndDestinationSame);
  FRIEND_TEST(S3MultiObjectCopyActionTest, ValidateCopyObjectRequestSuccess);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              FetchDestinationBucketInfoFailedMetadataMissing);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              FetchDestinationBucketInfoFailedFailedToLaunch);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              FetchDestinationBucketInfoFailedInternalError);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchDestinationObjectInfoFailed);
  FRIEND_TEST(S3MultiObjectCopyActionTest, FetchDestinationObjectInfoSuccess);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateObjectFirstAttempt);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateOneOrMoreObjects);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateObjectSecondAttempt);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateObjectFailedTestWhileShutdown);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              CreateObjectFailedWithCollisionExceededRetry);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateObjectFailedWithCollisionRetry);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateObjectFailedTest);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateObjectFailedToLaunchTest);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CreateNewOidTest);
  FRIEND_TEST(S3MultiObjectCopyActionTest, CopyFragments);
  FRIEND_TEST(S3MultiObjectCopyActionTest, ZeroSizeObject);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SaveMetadata);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SaveObjectMetadataFailed);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendErrorResponse);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendSuccessResponse);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendSuccessResponseAtEnd);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendSuccessResponseSpread);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendFailedResponse);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendFailedResponseAtEnd);
  FRIEND_TEST(S3MultiObjectCopyActionTest, SendFailedResponseSpread);
  FRIEND_TEST(S3MultiObjectCopyActionTest, DestinationAuthorization);
  FRIEND_TEST(S3MultiObjectCopyActionTest,
              DestinationAuthorizationSourceTagsAndACLHeader);
};
#endif  // __S3_SERVER_S3_MULTIOBJECT_COPY_ACTION_H__

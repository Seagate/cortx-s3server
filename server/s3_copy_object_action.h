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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest_prod.h>

#include "s3_put_object_action_base.h"
#include "s3_object_data_copier.h"
#include "s3_object_metadata.h"

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

class S3CopyObjectAction : public S3PutObjectActionBase {
  std::string source_bucket_name;
  std::string source_object_name;
  std::string auth_acl;

  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::shared_ptr<S3ObjectMetadata> source_object_metadata;
  std::shared_ptr<S3BucketMetadata> source_bucket_metadata;
  std::unique_ptr<S3ObjectDataCopier> object_data_copier;

  bool response_started = false;

  void get_source_bucket_and_object();
  void fetch_source_bucket_info();
  void fetch_source_bucket_info_success();
  void fetch_source_bucket_info_failed();

  void fetch_source_object_info();
  void fetch_source_object_info_success();
  void fetch_source_object_info_failed();

  bool if_source_and_destination_same();

  void set_authorization_meta();
  void check_source_bucket_authorization();
  void check_source_bucket_authorization_success();
  void check_source_bucket_authorization_failed();
  void set_source_bucket_authorization_metadata();
  void check_destination_bucket_authorization_failed();

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

  std::string get_response_xml();

  void validate_copyobject_request();
  void copy_object();
  bool copy_object_cb();
  void copy_object_success();
  void copy_object_failed();
  void save_metadata();
  void save_object_metadata_success();
  void save_object_metadata_failed();
  void start_response();
  void send_response_to_s3_client() final;

  friend class S3CopyObjectActionTest;

  FRIEND_TEST(S3CopyObjectActionTest, GetSourceBucketAndObjectSuccess);
  FRIEND_TEST(S3CopyObjectActionTest, GetSourceBucketAndSpecialObjectSuccess);
  FRIEND_TEST(S3CopyObjectActionTest, UIGetSourceBucketAndObjectSuccess);
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
              ValidateCopyObjectRequestMetadataDirectiveReplace);
  FRIEND_TEST(S3CopyObjectActionTest,
              ValidateCopyObjectRequestMetadataDirectiveInvalid);
  FRIEND_TEST(S3CopyObjectActionTest,
              ValidateCopyObjectRequestTaggingDirectiveReplace);
  FRIEND_TEST(S3CopyObjectActionTest,
              ValidateCopyObjectRequestTaggingDirectiveInvalid);
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
  FRIEND_TEST(S3CopyObjectActionTest, CreateObjectFirstAttempt);
  FRIEND_TEST(S3CopyObjectActionTest, CreateObjectSecondAttempt);
  FRIEND_TEST(S3CopyObjectActionTest, CreateObjectFailedTestWhileShutdown);
  FRIEND_TEST(S3CopyObjectActionTest,
              CreateObjectFailedWithCollisionExceededRetry);
  FRIEND_TEST(S3CopyObjectActionTest, CreateObjectFailedWithCollisionRetry);
  FRIEND_TEST(S3CopyObjectActionTest, CreateObjectFailedTest);
  FRIEND_TEST(S3CopyObjectActionTest, CreateObjectFailedToLaunchTest);
  FRIEND_TEST(S3CopyObjectActionTest, CreateNewOidTest);
  FRIEND_TEST(S3CopyObjectActionTest, ZeroSizeObject);
  FRIEND_TEST(S3CopyObjectActionTest, SaveMetadata);
  FRIEND_TEST(S3CopyObjectActionTest, SaveObjectMetadataFailed);
  FRIEND_TEST(S3CopyObjectActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3CopyObjectActionTest, SendErrorResponse);
  FRIEND_TEST(S3CopyObjectActionTest, SendSuccessResponse);
  FRIEND_TEST(S3CopyObjectActionTest, SendSuccessResponseAtEnd);
  FRIEND_TEST(S3CopyObjectActionTest, SendSuccessResponseSpread);
  FRIEND_TEST(S3CopyObjectActionTest, SendFailedResponse);
  FRIEND_TEST(S3CopyObjectActionTest, SendFailedResponseAtEnd);
  FRIEND_TEST(S3CopyObjectActionTest, SendFailedResponseSpread);
  FRIEND_TEST(S3CopyObjectActionTest, DestinationAuthorization);
};
#endif  // __S3_SERVER_S3_COPY_OBJECT_ACTION_H__

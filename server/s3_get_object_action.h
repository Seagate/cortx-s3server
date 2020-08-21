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

#ifndef __S3_SERVER_S3_GET_OBJECT_ACTION_H__
#define __S3_SERVER_S3_GET_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_object_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_motr_reader.h"
#include "s3_factory.h"
#include "s3_timer.h"

class S3GetObjectAction : public S3ObjectAction {

  std::shared_ptr<S3MotrReader> motr_reader;
  // Read state
  size_t total_blocks_in_object;
  size_t blocks_already_read;
  size_t data_sent_to_client;
  size_t content_length;

  size_t first_byte_offset_to_read;
  size_t last_byte_offset_to_read;
  size_t total_blocks_to_read;

  bool read_object_reply_started;
  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  S3Timer s3_timer;

  size_t get_requested_content_length() const {
    return last_byte_offset_to_read - first_byte_offset_to_read + 1;
  }

 public:
  S3GetObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3MotrReaderFactory> motr_s3_factory = nullptr);

  void setup_steps();

  void fetch_bucket_info_failed();

  void fetch_object_info_failed();
  void validate_object_info();
  void check_full_or_range_object_read();
  void set_total_blocks_to_read_from_object();
  bool validate_range_header_and_set_read_options(
      const std::string& range_value);
  void read_object();

  void read_object_data();
  void read_object_data_failed();
  void send_data_to_client();
  void send_response_to_s3_client();

  FRIEND_TEST(S3GetObjectActionTest, ConstructorTest);
  FRIEND_TEST(S3GetObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3GetObjectActionTest, FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3GetObjectActionTest, FetchObjectInfoWhenBucketFetchFailed);
  FRIEND_TEST(S3GetObjectActionTest,
              FetchObjectInfoWhenBucketFetchFailedToLaunch);
  FRIEND_TEST(S3GetObjectActionTest,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3GetObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3GetObjectActionTest,
              ValidateObjectWhenMissingObjectReportNoSuckKey);
  FRIEND_TEST(S3GetObjectActionTest,
              ValidateObjectWhenObjInfoFetchFailedReportError);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectFailedJustEndResponse1);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectFailedJustEndResponse2);
  FRIEND_TEST(S3GetObjectActionTest, ValidateObjectOfSizeZero);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeLessThanUnitSize);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeEqualToUnitSize);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeMoreThanUnitSize);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfGivenRange);
  FRIEND_TEST(S3GetObjectActionTest,
              SendResponseWhenShuttingDownAndResponseStarted);
  FRIEND_TEST(S3GetObjectActionTest,
              SendResponseWhenShuttingDownAndResponseNotStarted);
  FRIEND_TEST(S3GetObjectActionTest, SendInternalErrorResponse);
  FRIEND_TEST(S3GetObjectActionTest, SendNoSuchBucketErrorResponse);
  FRIEND_TEST(S3GetObjectActionTest, SendNoSuchKeyErrorResponse);
  FRIEND_TEST(S3GetObjectActionTest, SendSuccessResponseForZeroSizeObject);
  FRIEND_TEST(S3GetObjectActionTest, SendSuccessResponseForNonZeroSizeObject);
  FRIEND_TEST(S3GetObjectActionTest, SendErrorResponseForErrorReadingObject);
  FRIEND_TEST(S3GetObjectActionTest, CheckFullOrRangeObjectReadWithEmptyRange);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithValidRangeFirst500ForContentLength8000);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithValidRangewithspacesForContentLength8000_1);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithValidRangewithspacesForContentLength8000_2);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithValidRangeLast500ForContentLength8000);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithValidRangeLast500WithSpaceForContentLength8000);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadLastByteForContentLength8000);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadLastBytewithIncludeSpaceForContentLength8000);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadFirstByteForContentLength8000);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithRangeInMultipleBlocksForContentLength8000);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadFromGivenOffsetForContentLength8000_1);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadFromGivenOffsetForContentLength8000_2);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadWithInvalidRangeForContentLength8000_1);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadWithInvalidRangeForContentLength8000_2);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadWithInvalidRangeForContentLength8000_3);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadWithInvalidRangeForContentLength8000_4);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadWithInvalidRangeForContentLength8000_5);
  FRIEND_TEST(S3GetObjectActionTest,
              CheckFullOrRangeObjectReadWithInvalidRangeForContentLength8000_6);
  FRIEND_TEST(
      S3GetObjectActionTest,
      CheckFullOrRangeObjectReadWithUnsupportMultiRangeForContentLength8000);
};

#endif

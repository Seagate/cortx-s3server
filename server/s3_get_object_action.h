/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __S3_SERVER_S3_GET_OBJECT_ACTION_H__
#define __S3_SERVER_S3_GET_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_reader.h"
#include "s3_factory.h"
#include "s3_object_metadata.h"

class S3GetObjectAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ClovisReader> clovis_reader;
  m0_uint128 object_list_oid;
  // Read state
  size_t total_blocks_in_object;
  size_t blocks_already_read;
  size_t data_sent_to_client;

  bool read_object_reply_started;

  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;
  std::shared_ptr<S3ClovisReaderFactory> clovis_reader_factory;

 public:
  S3GetObjectAction(std::shared_ptr<S3RequestObject> req,
                    S3BucketMetadataFactory* bucket_meta_factory = NULL,
                    S3ObjectMetadataFactory* object_meta_factory = NULL,
                    S3ClovisReaderFactory* clovis_s3_factory = NULL);

  void setup_steps();

  void fetch_bucket_info();
  void fetch_object_info();
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
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3GetObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3GetObjectActionTest,
              ReadObjectWhenMissingObjectReportNoSuckKey);
  FRIEND_TEST(S3GetObjectActionTest,
              ReadObjectWhenObjInfoFetchFailedReportError);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectFailedJustEndResponse);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeZero);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeLessThanUnitSize);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeEqualToUnitSize);
  FRIEND_TEST(S3GetObjectActionTest, ReadObjectOfSizeMoreThanUnitSize);
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
};

#endif

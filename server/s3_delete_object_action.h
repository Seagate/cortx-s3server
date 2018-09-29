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

#ifndef __S3_SERVER_S3_DELETE_OBJECT_ACTION_H__
#define __S3_SERVER_S3_DELETE_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_factory.h"
#include "s3_log.h"
#include "s3_object_metadata.h"

class S3DeleteObjectAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ClovisWriter> clovis_writer;

  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3ObjectMetadataFactory> object_metadata_factory;
  std::shared_ptr<S3ClovisWriterFactory> clovis_writer_factory;

 public:
  S3DeleteObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr,
      std::shared_ptr<S3ClovisWriterFactory> writer_factory = nullptr);

  void setup_steps();

  void fetch_bucket_info();
  void fetch_object_info();
  void delete_metadata();
  void delete_object();
  void delete_object_failed();
  void send_response_to_s3_client();

  FRIEND_TEST(S3DeleteObjectActionTest, ConstructorTest);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketFetchFailed);
  FRIEND_TEST(S3DeleteObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3DeleteObjectActionTest,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteMetadataWhenObjectPresent);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteMetadataWhenObjectAbsent);
  FRIEND_TEST(S3DeleteObjectActionTest,
              DeleteMetadataWhenObjectMetadataFetchFailed);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteObjectWhenMetadataDeleteFailed);
  FRIEND_TEST(S3DeleteObjectActionTest,
              DeleteObjectWhenMetadataDeleteSucceeded);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteObjectMissingShouldReportDeleted);
  FRIEND_TEST(S3DeleteObjectActionTest, DeleteObjectFailedShouldReportDeleted);
  FRIEND_TEST(S3DeleteObjectActionTest, SendErrorResponse);
  FRIEND_TEST(S3DeleteObjectActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3DeleteObjectActionTest, SendSuccessResponse);
};

#endif

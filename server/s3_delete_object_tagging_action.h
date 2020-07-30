#pragma once

#ifndef __S3_SERVER_S3_DELETE_OBJECT_TAGGING_ACTION_H__
#define __S3_SERVER_S3_DELETE_OBJECT_TAGGING_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>

#include "s3_factory.h"
#include "s3_object_action_base.h"
#include "s3_object_metadata.h"

class S3DeleteObjectTaggingAction : public S3ObjectAction {

 public:
  S3DeleteObjectTaggingAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  void setup_steps();
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void delete_object_tags();
  void delete_object_tags_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, Constructor);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, FetchBucketInfo);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              FetchBucketInfoFailedNoSuchBucket);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, GetObjectMetadataFailedMissing);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              GetObjectMetadataFailedInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, DeleteTag);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, DeleteTagFailed);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, DeleteTagFailedInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest,
              SendResponseToClientInternalError);
  FRIEND_TEST(S3DeleteObjectTaggingActionTest, SendResponseToClientSuccess);
};

#endif

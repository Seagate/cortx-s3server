#pragma once

#ifndef __S3_SERVER_S3_GET_OBJECT_TAGGING_ACTION_H__
#define __S3_SERVER_S3_GET_OBJECT_TAGGING_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_object_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3GetObjectTaggingAction : public S3ObjectAction {

 public:
  S3GetObjectTaggingAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  void setup_steps();
  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void send_response_to_s3_client();

  FRIEND_TEST(S3GetObjectTaggingActionTest, Constructor);
  FRIEND_TEST(S3GetObjectTaggingActionTest, FetchBucketInfoFailedNoSuchBucket);
  FRIEND_TEST(S3GetObjectTaggingActionTest, FetchBucketInfoFailedInternalError);
  FRIEND_TEST(S3GetObjectTaggingActionTest, GetObjectMetadataEmpty);
  FRIEND_TEST(S3GetObjectTaggingActionTest, GetObjectMetadataFailedMissing);
  FRIEND_TEST(S3GetObjectTaggingActionTest,
              GetObjectMetadataFailedInternalError);
  FRIEND_TEST(S3GetObjectTaggingActionTest, SendResponseToClientEmptyTagSet);
  FRIEND_TEST(S3GetObjectTaggingActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3GetObjectTaggingActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3GetObjectTaggingActionTest, SendResponseToClientSuccess);
};

#endif

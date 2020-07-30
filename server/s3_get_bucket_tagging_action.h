#pragma once

#ifndef __S3_SERVER_S3_GET_BUCKET_TAGGING_ACTION_H__
#define __S3_SERVER_S3_GET_BUCKET_TAGGING_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3GetBucketTaggingAction : public S3BucketAction {

 public:
  S3GetBucketTaggingAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr);

  void setup_steps();
  void check_metadata_missing_status();
  void fetch_bucket_info_failed();
  void send_response_to_s3_client();

  // google unit tests
  FRIEND_TEST(S3GetBucketTaggingActionTest,
              SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3GetBucketTaggingActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3GetBucketTaggingActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3GetBucketTaggingActionTest, SendResponseToClientInternalError);
  FRIEND_TEST(S3GetBucketTaggingActionTest,
              SendResponseToClientNoSuchTagSetError);
};

#endif

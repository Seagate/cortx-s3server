#pragma once

#ifndef __S3_SERVER_S3_GET_BUCKET_LOCATION_ACTION_H__
#define __S3_SERVER_S3_GET_BUCKET_LOCATION_ACTION_H__

#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"

class S3GetBucketlocationAction : public S3BucketAction {

 public:
  S3GetBucketlocationAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr);

  void setup_steps();

  virtual void fetch_bucket_info_failed();
  virtual void send_response_to_s3_client();

  FRIEND_TEST(S3GetBucketLocationActionTest, BucketMetadataMustNotBeNull);
  FRIEND_TEST(S3GetBucketLocationActionTest, FetchBucketInfoFailedWithMissing);
  FRIEND_TEST(S3GetBucketLocationActionTest, FetchBucketInfoFailedWithFailed);
  FRIEND_TEST(S3GetBucketLocationActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3GetBucketLocationActionTest, SendErrorResponse);
  FRIEND_TEST(S3GetBucketLocationActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3GetBucketLocationActionTest, SendAnySuccessResponse);
};
#endif

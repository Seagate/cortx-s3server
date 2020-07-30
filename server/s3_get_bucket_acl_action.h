#pragma once

#ifndef __S3_SERVER_S3_GET_BUCKET_ACL_ACTION_H__
#define __S3_SERVER_S3_GET_BUCKET_ACL_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"

class S3GetBucketACLAction : public S3BucketAction {

 public:
  S3GetBucketACLAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr);

  void setup_steps();

  void fetch_bucket_info_failed();
  void send_response_to_s3_client();

  FRIEND_TEST(S3GetBucketAclActionTest, Constructor);
  FRIEND_TEST(S3GetBucketAclActionTest, FetchBucketInfoFailedWithMissing);
  FRIEND_TEST(S3GetBucketAclActionTest, FetchBucketInfoFailed);
  FRIEND_TEST(S3GetBucketAclActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3GetBucketAclActionTest, SendErrorResponse);
  FRIEND_TEST(S3GetBucketAclActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3GetBucketAclActionTest, SendSuccessResponse);
};

#endif

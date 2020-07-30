#pragma once

#ifndef __S3_SERVER_S3_HEAD_OBJECT_ACTION_H__
#define __S3_SERVER_S3_HEAD_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_object_action_base.h"

class S3HeadObjectAction : public S3ObjectAction {

 public:
  S3HeadObjectAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory = nullptr);

  void setup_steps();

  void fetch_bucket_info_failed();
  void fetch_object_info_failed();
  void send_response_to_s3_client();

  FRIEND_TEST(S3HeadObjectActionTest, ConstructorTest);
  FRIEND_TEST(S3HeadObjectActionTest, FetchBucketInfo);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoWhenBucketNotPresent);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoWhenBucketFetchFailed);
  FRIEND_TEST(S3HeadObjectActionTest,
              FetchObjectInfoWhenBucketFetchFailedToLaunch);
  FRIEND_TEST(S3HeadObjectActionTest,
              FetchObjectInfoWhenBucketAndObjIndexPresent);
  FRIEND_TEST(S3HeadObjectActionTest,
              FetchObjectInfoWhenBucketPresentAndObjIndexAbsent);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoReturnedMissing);
  FRIEND_TEST(S3HeadObjectActionTest, FetchObjectInfoFailedWithError);
  FRIEND_TEST(S3HeadObjectActionTest, SendResponseWhenShuttingDown);
  FRIEND_TEST(S3HeadObjectActionTest, SendErrorResponse);
  FRIEND_TEST(S3HeadObjectActionTest, SendAnyFailedResponse);
  FRIEND_TEST(S3HeadObjectActionTest, SendSuccessResponse);
};

#endif

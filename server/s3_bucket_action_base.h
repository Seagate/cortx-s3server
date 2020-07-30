#pragma once

#ifndef __S3_SERVER_S3_BUCKET_ACTION_BASE_H__
#define __S3_SERVER_S3_BUCKET_ACTION_BASE_H__

#include <functional>
#include <memory>
#include <vector>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"
#include "s3_fi_common.h"
#include "s3_log.h"

/*
   All the bucket action classes (PUT, GET etc..) need to derive from
   S3BucketAction class. This class provides metdata fetch functions
   required for bucket action authorization and bucket action processing
   S3 API's that require Bucket ACL for authorization, need to derive from
   S3BucketAction Class.

   Derived bucket action classes need to implement below functions
   fetch_bucket_info_failed(): handle failures if bucket metdata doesnt exist

 */
class S3BucketAction : public S3Action {

 protected:
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  void fetch_bucket_info();
  virtual void fetch_bucket_info_failed() = 0;

 public:
  S3BucketAction(std::shared_ptr<S3RequestObject> req,
                 std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory =
                     nullptr,
                 bool check_shutdown = true,
                 std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
                 bool skip_auth = false);
  virtual ~S3BucketAction();

  void load_metadata();
  void set_authorization_meta();

  FRIEND_TEST(S3BucketActionTest, Constructor);
  FRIEND_TEST(S3BucketActionTest, FetchBucketInfo);
  FRIEND_TEST(S3BucketActionTest, LoadMetadata);
  FRIEND_TEST(S3BucketActionTest, SetAuthorizationMeta);
};

#endif

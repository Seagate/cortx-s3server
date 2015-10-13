#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_HEAD_OBJECT_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_HEAD_OBJECT_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_object_metadata.h"

class S3HeadObjectAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;

public:
  S3HeadObjectAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void fetch_bucket_info();
  void fetch_object_info();
  void send_response_to_s3_client();
};

#endif

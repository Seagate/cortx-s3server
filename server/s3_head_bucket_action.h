
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_HEAD_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_HEAD_BUCKET_ACTION_H__

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"

class S3HeadBucketAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

public:
  S3HeadBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void read_metadata();
  void send_response_to_s3_client();
};

#endif

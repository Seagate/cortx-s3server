
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_PUT_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_PUT_BUCKET_ACTION_H__

#include "s3_action_base.h"

class S3PutBucketAction : public S3Action {
  // S3BucketMetadata* metadata;

public:
  S3PutBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

};

#endif

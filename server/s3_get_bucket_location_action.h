#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_GET_BUCKET_LOCATION_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_GET_BUCKET_LOCATION_ACTION_H__

#include <memory>

#include "s3_action_base.h"
// #include "s3_bucket_metadata.h"
// #include "s3_clovis_kv_reader.h"

class S3GetBucketlocationAction : public S3Action {
  // std::shared_ptr<S3BucketMetadata> metadata;

public:
  S3GetBucketlocationAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void get_metadata();
  void send_response_to_s3_client();
};

#endif

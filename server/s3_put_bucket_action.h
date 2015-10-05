
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_PUT_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_PUT_BUCKET_ACTION_H__

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"

class S3PutBucketAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

  std::string location_constraint;  // Received in request body.
public:
  S3PutBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void validate_request();
  void read_metadata();
  void create_bucket();
  void send_response_to_s3_client();
};

#endif

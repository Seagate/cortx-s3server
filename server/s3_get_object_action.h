#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_GET_OBJECT_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_GET_OBJECT_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_object_metadata.h"
#include "s3_clovis_reader.h"

class S3GetObjectAction : public S3Action {
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ClovisReader> clovis_reader;

public:
  S3GetObjectAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void fetch_metadata();
  void read_object();
  void read_object_failed();
  void send_response_to_s3_client();
};

#endif

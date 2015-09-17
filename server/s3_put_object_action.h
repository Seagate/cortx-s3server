#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_PUT_OBJECT_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_PUT_OBJECT_ACTION_H__

#include <memory>

#include "s3_action_base.h"
// #include "s3_object_metadata.h"
#include "s3_clovis_writer.h"

class S3PutObjectAction : public S3Action {
  // std::shared_ptr<S3ObjectMetadata> metadata;
  std::shared_ptr<S3ClovisWriter> clovis_writer;

public:
  S3PutObjectAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void fetch_metadata();
  void write_metadata();
  void create_object();
  void create_object_failed();
  void write_object();
  void write_object_failed();
  void send_response_to_s3_client();
};

#endif

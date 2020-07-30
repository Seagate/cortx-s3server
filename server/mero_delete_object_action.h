#pragma once

#ifndef __MERO_DELETE_OBJECT_ACTION_H__
#define __MERO_DELETE_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_factory.h"
#include "mero_action_base.h"

class MeroDeleteObjectAction : public MeroAction {
  int layout_id;
  m0_uint128 oid;
  std::shared_ptr<S3ClovisWriter> clovis_writer;

  std::shared_ptr<S3ClovisWriterFactory> clovis_writer_factory;

 public:
  MeroDeleteObjectAction(std::shared_ptr<MeroRequestObject> req,
                         std::shared_ptr<S3ClovisWriterFactory> writer_factory =
                             nullptr);

  void setup_steps();
  void validate_request();

  void delete_object();
  void delete_object_successful();
  void delete_object_failed();

  void send_response_to_s3_client();
};
#endif
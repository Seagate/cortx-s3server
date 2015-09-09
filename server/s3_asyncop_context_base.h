
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_ASYNCOP_CONTEXT_BASE_H__
#define __MERO_FE_S3_SERVER_S3_ASYNCOP_CONTEXT_BASE_H__

#include <functional>
#include <memory>
#include <string>

#include "s3_common.h"
#include "s3_request_object.h"

class S3AsyncOpContextBase {
  std::shared_ptr<S3RequestObject> request;
  std::function<void(void)> on_success;
  std::function<void(void)> on_failed;

  // Operational details
  S3AsyncOpStatus status;
  std::string error_message;
public:
  S3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req, std::function<void(void)> success, std::function<void(void)> failed);
  ~S3AsyncOpContextBase() {}

  S3RequestObject* get_request();

  std::function<void(void)> on_success_handler();
  std::function<void(void)> on_failed_handler();

  S3AsyncOpStatus get_op_status();

  void set_op_status(S3AsyncOpStatus opstatus, std::string& message);

  std::string& get_error_message();

  virtual void consume(char* chars, size_t length) = 0;
};

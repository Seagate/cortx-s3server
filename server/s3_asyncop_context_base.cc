

#include "s3_asyncop_context_base.h"

S3AsyncOpContextBase::S3AsyncOpContextBase(S3RequestObject *req, std::function<void(void)> handler) : request(req), handler(handler), status(S3AsyncOpStatus::unknown) {

}

S3RequestObject* S3AsyncOpContextBase::get_request() {
  return request;
}

std::function<void(void)> S3AsyncOpContextBase::handler() {
  return handler;
}

S3AsyncOpStatus S3AsyncOpContextBase::get_op_status() {
  return status;
}

void S3AsyncOpContextBase::set_op_status(S3AsyncOpStatus opstatus, std::string& message) {
  status = opstatus;
  error_message = message;
}

std::string& S3AsyncOpContextBase::get_error_message() {
  return error_message;
}

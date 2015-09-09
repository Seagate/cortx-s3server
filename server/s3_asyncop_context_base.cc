

#include "s3_asyncop_context_base.h"

S3AsyncOpContextBase::S3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req, std::function<void(void)> success, std::function<void(void)> failed) : request(req), on_success(success), on_failed(failed), status(S3AsyncOpStatus::unknown) {

}

std::shared_ptr<S3RequestObject> S3AsyncOpContextBase::get_request() {
  return request;
}

std::function<void(void)> S3AsyncOpContextBase::on_success_handler() {
  return on_success;
}

std::function<void(void)> S3AsyncOpContextBase::on_failed_handler() {
  return on_failed;
}

S3AsyncOpStatus S3AsyncOpContextBase::get_op_status() {
  return status;
}

void S3AsyncOpContextBase::set_op_status(S3AsyncOpStatus opstatus, std::string message) {
  status = opstatus;
  error_message = message;
}

std::string& S3AsyncOpContextBase::get_error_message() {
  return error_message;
}

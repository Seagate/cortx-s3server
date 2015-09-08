
#include <functional>

#include "s3_common.h"
#include "s3_request_object.h"

class S3AsyncOpContextBase {
  S3RequestObject* request;
  std::function<void(void)> handler;

  // Operational details
  S3AsyncOpStatus status;
  std::string error_message;
public:
  S3AsyncOpContextBase(S3RequestObject *req, std::function<void(void)> handler);
  ~S3AsyncOpContextBase() {}

  S3RequestObject* get_request();

  std::function<void(void)> handler();

  S3AsyncOpStatus get_op_status();

  void set_op_status(S3AsyncOpStatus opstatus, std::string& message);

  std::string& get_error_message();

  virtual void consume(char* chars, size_t length) = 0;
};

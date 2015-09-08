
#include "s3_request_object.h"

class S3APIHandler {
  S3RequestObject* request;
  S3OperationCode operation_code;
  std::shared_ptr<S3APIHandler> self_ref;
public:
  virtual ~S3APIHandler();
  virtual void dispatch() = 0;

  // Self destructing object.
  void manage_self(std::shared_ptr<S3APIHandler> ref) {
      self_ref = ref;
  }
  // This *MUST* be the last thing on object. Called @ end of dispatch.
  void i_am_done() {
    self_ref.reset();
  }
};

class S3ServiceAPIHandler : public S3APIHandler {
public:
  virtual void dispatch();
}

class S3BucketAPIHandler : public S3APIHandler {
public:
  virtual void dispatch();
};

class S3ObjectAPIHandler : public S3APIHandler {
public:
  virtual void dispatch();
};

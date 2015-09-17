
#include "s3_api_handler.h"
#include "s3_put_object_action.h"

void S3ObjectAPIHandler::dispatch() {
  std::shared_ptr<S3Action> action;
  switch(operation_code) {
    case S3OperationCode::acl:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          // action = std::make_shared<S3GetObjectACLAction>(request);
          break;
        case S3HttpVerb::PUT:
          // action = std::make_shared<S3PutObjectACLAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    case S3OperationCode::none:
      // Perform operation on Object.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          // action = std::make_shared<S3HeadObjectAction>(request);
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutObjectAction>(request);
          break;
        case S3HttpVerb::GET:
          // action = std::make_shared<S3GetObjectAction>(request);
          break;
        case S3HttpVerb::DELETE:
          // action = std::make_shared<S3DeleteObjectAction>(request);
          break;
        case S3HttpVerb::POST:
          // action = std::make_shared<S3PostObjectAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    default:
      // should never be here.
      request->respond_unsupported_api();
      i_am_done();
      return;
  };  // switch operation_code
  action->manage_self(action);
  action->start();
  i_am_done();
}


#include "s3_api_handler.h"

void S3ObjectAPIHandler::dispatch() {
  S3Action* action = NULL;
  switch(operation_code) {
    case S3OperationCode::acl:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = new S3GetObjectACLAction(request);
          break;
        case S3HttpVerb::PUT:
          action = new S3PutObjectACLAction(request);
          break;
        default:
          // should never be here.
          action = new S3UnknowAPIAction(request);
          action.error_message("Unsupported HTTP operation on Object ACL.")
      };
      break;
    case S3OperationCode::none:
      // Perform operation on Object.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          action = new S3HeadObjectAction(request);
          break;
        case S3HttpVerb::PUT:
          action = new S3PutObjectAction(request);
          break;
        case S3HttpVerb::DELETE:
          action = new S3DeleteObjectAction(request);
          break;
        case S3HttpVerb::POST:
          action = new S3PostObjectAction(request);
          break;
        default:
          // should never be here.
          action = new S3UnknowAPIAction(request);
          action.error_message("Unsupported HTTP operation on Object.")
      };
      break;
  };  // switch operation_code
  action.start()
  i_am_done();
}

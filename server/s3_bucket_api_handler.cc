
#include "s3_api_handler.h"

void S3BucketAPIHandler::dispatch() {
  S3Action* action = NULL;
  switch(operation_code) {
    printf("Action operation code = %d\n",operation_code);
    case S3OperationCode::list:
      action = new S3BucketListingAction(request);
      break;
    case S3OperationCode::acl:
      // ACL operations.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = new S3GetBucketACLAction(request);
          break;
        case S3HttpVerb::PUT:
          action = new S3PutBucketACLAction(request);
          break;
        default:
          // should never be here.
          action = new S3UnknowAPIAction(request);
          action.error_message("Unsupported HTTP operation on Bucket ACL.")
      };
      break;
    case S3OperationCode::none:
      // Perform operation on Bucket.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          action = new S3HeadBucketAction(request);
          break;
        case S3HttpVerb::PUT:
          action = new S3PutBucketAction(request);
          break;
        case S3HttpVerb::DELETE:
          action = new S3DeleteBucketAction(request);
          break;
        case S3HttpVerb::POST:
          action = new S3PostBucketAction(request);
          break;
        default:
          // should never be here.
          action = S3UnknowAPIAction(request);
          action.error_message("Unsupported HTTP operation on Bucket.")
      };
  };  // switch operation_code
  action.start()
  i_am_done();
}

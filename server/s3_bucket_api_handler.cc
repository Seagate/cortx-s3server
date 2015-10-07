
#include "s3_api_handler.h"
#include "s3_get_bucket_location_action.h"
#include "s3_head_bucket_action.h"
#include "s3_put_bucket_action.h"

void S3BucketAPIHandler::dispatch() {
  std::shared_ptr<S3Action> action;
  printf("Action operation code = %d\n", operation_code);
  switch(operation_code) {
    case S3OperationCode::location:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetBucketlocationAction>(request);
          break;
        case S3HttpVerb::PUT:
          // action = std::make_shared<S3PutBucketlocationAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    case S3OperationCode::list:
      // action = std::make_shared<S3BucketListingAction>(request);
      break;
    case S3OperationCode::acl:
      // ACL operations.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          // action = std::make_shared<S3GetBucketACLAction>(request);
          break;
        case S3HttpVerb::PUT:
          // action = std::make_shared<S3PutBucketACLAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    case S3OperationCode::none:
      // Perform operation on Bucket.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          action = std::make_shared<S3HeadBucketAction>(request);
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutBucketAction>(request);
          break;
        case S3HttpVerb::DELETE:
          // action = std::make_shared<S3DeleteBucketAction>(request);
          break;
        case S3HttpVerb::POST:
          // action = std::make_shared<S3PostBucketAction>(request);
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
  if (action) {
    action->manage_self(action);
    action->start();
  } else {
    request->respond_unsupported_api();
  }
  i_am_done();
}

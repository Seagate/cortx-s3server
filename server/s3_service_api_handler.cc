
#include "s3_api_handler.h"
#include "s3_get_service_action.h"

void S3ServiceAPIHandler::dispatch() {
  std::shared_ptr<S3Action> action;
  printf("Action operation code = %d\n", operation_code);
  switch(operation_code) {
    case S3OperationCode::none:
      // Perform operation on Service.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetServiceAction>(request);
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

#include "s3_action_base.h"
#include "s3_api_handler.h"
#include "s3_account_delete_metadata_action.h"

void S3ManagementAPIHandler::create_action() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering");
  s3_log(S3_LOG_DEBUG, request_id, "Action operation code = %d\n",
         operation_code);
  switch (operation_code) {
    case S3OperationCode::none:
      // Perform operation on Service.
      switch (request->http_verb()) {
        case S3HttpVerb::DELETE:
          action = std::make_shared<S3AccountDeleteMetadataAction>(request);
          s3_log(S3_LOG_DEBUG, request_id, "S3AccountDeleteMetadataAction");
          break;
        default:
          // should never be here.
          return;
      };
      break;
    default:
      // should never be here.
      return;
  };  // switch operation_code
  s3_log(S3_LOG_DEBUG, request_id, "Exiting");
}

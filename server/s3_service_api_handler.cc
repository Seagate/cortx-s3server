#include "s3_api_handler.h"
#include "s3_get_service_action.h"
#include "s3_head_service_action.h"
#include "s3_stats.h"

void S3ServiceAPIHandler::create_action() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Action operation code = %d\n",
         operation_code);

  switch (operation_code) {
    case S3OperationCode::none:
      // Perform operation on Service.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          request->set_action_str("ListAllMyBuckets");
          action = std::make_shared<S3GetServiceAction>(request);
          s3_stats_inc("get_service_request_count");
          break;
        case S3HttpVerb::HEAD:
          request->set_action_str("HeadService");
          action = std::make_shared<S3HeadServiceAction>(request);
          s3_stats_inc("health_check_request_count");
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

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

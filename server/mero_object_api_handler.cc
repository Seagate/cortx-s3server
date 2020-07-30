#include "mero_api_handler.h"
#include "mero_delete_object_action.h"
#include "mero_head_object_action.h"
#include "s3_log.h"
#include "s3_stats.h"

void MeroObjectAPIHandler::create_action() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Operation code = %d\n", operation_code);

  switch (operation_code) {
    case MeroOperationCode::none:
      // Perform operation on Object.
      switch (request->http_verb()) {
        case S3HttpVerb::DELETE:
          action = std::make_shared<MeroDeleteObjectAction>(request);
          s3_stats_inc("mero_http_delete_object_request_count");
          break;
        case S3HttpVerb::HEAD:
          action = std::make_shared<MeroHeadObjectAction>(request);
          s3_stats_inc("mero_http_head_object_request_count");
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

#include "mero_api_handler.h"
#include "mero_get_key_value_action.h"
#include "mero_put_key_value_action.h"
#include "mero_delete_key_value_action.h"
#include "s3_log.h"
#include "s3_stats.h"

void MeroKeyValueAPIHandler::create_action() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Operation code = %d\n", operation_code);

  switch (operation_code) {
    case MeroOperationCode::none:
      // Perform operation on Object.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<MeroGetKeyValueAction>(request);
          s3_stats_inc("mero_http_get_keyvalue_request_count");
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<MeroPutKeyValueAction>(request);
          s3_stats_inc("mero_http_put_keyvalue_request_count");
          break;
        case S3HttpVerb::DELETE:
          action = std::make_shared<MeroDeleteKeyValueAction>(request);
          s3_stats_inc("mero_http_delete_keyvalue_request_count");
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

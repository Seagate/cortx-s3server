#include "s3_api_handler.h"
#include "s3_log.h"

std::shared_ptr<S3APIHandler> S3APIHandlerFactory::create_api_handler(
    S3ApiType api_type, std::shared_ptr<S3RequestObject> request,
    S3OperationCode op_code) {
  std::shared_ptr<S3APIHandler> handler;
  std::string request_id = request->get_request_id();
  request->set_operation_code(op_code);
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  switch (api_type) {
    case S3ApiType::service:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = S3ApiType::service\n");
      handler = std::make_shared<S3ServiceAPIHandler>(request, op_code);
      break;
    case S3ApiType::bucket:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = S3ApiType::bucket\n");
      handler = std::make_shared<S3BucketAPIHandler>(request, op_code);
      break;
    case S3ApiType::object:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = S3ApiType::object\n");
      handler = std::make_shared<S3ObjectAPIHandler>(request, op_code);
      break;
    case S3ApiType::faultinjection:
      s3_log(S3_LOG_DEBUG, request_id,
             "api_type = S3ApiType::faultinjection\n");
      handler = std::make_shared<S3FaultinjectionAPIHandler>(request, op_code);
      break;
    case S3ApiType::management:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = S3ApiType::management\n");
      handler = std::make_shared<S3ManagementAPIHandler>(request, op_code);
      break;
    default:
      break;
  };
  if (handler) {
    s3_log(S3_LOG_DEBUG, request_id, "HTTP Action is %s\n",
           request->get_http_verb_str(request->http_verb()));
    handler->create_action();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return handler;
}

#include "mero_api_handler.h"
#include "s3_log.h"

std::shared_ptr<MeroAPIHandler> MeroAPIHandlerFactory::create_api_handler(
    MeroApiType api_type, std::shared_ptr<MeroRequestObject> request,
    MeroOperationCode op_code) {
  std::shared_ptr<MeroAPIHandler> handler;
  std::string request_id = request->get_request_id();
  request->set_operation_code(op_code);
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  switch (api_type) {
    case MeroApiType::index:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = MeroApiType::index\n");
      handler = std::make_shared<MeroIndexAPIHandler>(request, op_code);
      break;
    case MeroApiType::keyval:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = MeroApiType::keyval\n");
      handler = std::make_shared<MeroKeyValueAPIHandler>(request, op_code);
      break;
    case MeroApiType::object:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = MeroApiType::object\n");
      handler = std::make_shared<MeroObjectAPIHandler>(request, op_code);
      break;
    case MeroApiType::faultinjection:
      s3_log(S3_LOG_DEBUG, request_id,
             "api_type = MeroApiType::faultinjection\n");
      // TODO
      // handler = std::make_shared<MeroFaultinjectionAPIHandler>(request,
      // op_code);
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

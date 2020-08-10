/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include "motr_api_handler.h"
#include "s3_log.h"

std::shared_ptr<MotrAPIHandler> MotrAPIHandlerFactory::create_api_handler(
    MotrApiType api_type, std::shared_ptr<MotrRequestObject> request,
    MotrOperationCode op_code) {
  std::shared_ptr<MotrAPIHandler> handler;
  std::string request_id = request->get_request_id();
  request->set_operation_code(op_code);
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  switch (api_type) {
    case MotrApiType::index:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = MotrApiType::index\n");
      handler = std::make_shared<MotrIndexAPIHandler>(request, op_code);
      break;
    case MotrApiType::keyval:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = MotrApiType::keyval\n");
      handler = std::make_shared<MotrKeyValueAPIHandler>(request, op_code);
      break;
    case MotrApiType::object:
      s3_log(S3_LOG_DEBUG, request_id, "api_type = MotrApiType::object\n");
      handler = std::make_shared<MotrObjectAPIHandler>(request, op_code);
      break;
    case MotrApiType::faultinjection:
      s3_log(S3_LOG_DEBUG, request_id,
             "api_type = MotrApiType::faultinjection\n");
      // TODO
      // handler = std::make_shared<MotrFaultinjectionAPIHandler>(request,
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

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

#include <string>
#include "s3_error_codes.h"
#include "s3_head_service_action.h"
#include "s3_iem.h"
#include "s3_log.h"

S3HeadServiceAction::S3HeadServiceAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req, true, nullptr, true, true) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  setup_steps();
}

void S3HeadServiceAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3HeadServiceAction::send_response_to_s3_client, this);
  // ...
}

void S3HeadServiceAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  // Disable Audit logs for Haproxy healthchecks
  const char* full_path_uri = request->c_get_full_path();

  if (full_path_uri) {
    if (std::strcmp(full_path_uri, "/") == 0) {
      request->get_audit_info().set_publish_flag(false);
    }
  }
  if (reject_if_shutting_down()) {
    int shutdown_grace_period =
        S3Option::get_instance()->get_s3_grace_period_sec();
    request->set_out_header_value("Retry-After",
                                   std::to_string(shutdown_grace_period));
    request->set_out_header_value("Connection", "close");
    request->send_response(S3HttpFailed503);
  } else {
    request->send_response(S3HttpSuccess200);
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

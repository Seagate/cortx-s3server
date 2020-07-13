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

#include "motr_action_base.h"
#include "s3_error_codes.h"
#include "s3_option.h"
#include "s3_stats.h"

MotrAction::MotrAction(std::shared_ptr<MotrRequestObject> req,
                       bool check_shutdown,
                       std::shared_ptr<S3AuthClientFactory> auth_factory,
                       bool skip_auth)
    : Action(req, check_shutdown, auth_factory, skip_auth), request(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  auth_client->do_skip_authorization();
  setup_steps();
}

MotrAction::~MotrAction() { s3_log(S3_LOG_DEBUG, request_id, "Destructor\n"); }

void MotrAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "S3Option::is_auth_disabled: (%d), skip_auth: (%d)\n",
         S3Option::get_instance()->is_auth_disabled(), skip_auth);

  if (!S3Option::get_instance()->is_auth_disabled() && !skip_auth) {
    ACTION_TASK_ADD(MotrAction::check_authorization, this);
  }
}

void MotrAction::check_authorization() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if ((request->get_account_name() == BACKGROUND_STALE_OBJECT_DELETE_ACCOUNT) ||
      (request->get_account_name() == S3RECOVERY_ACCOUNT)) {
    next();
  } else {
    if (request->client_connected()) {
      std::string error_code = "InvalidAccessKeyId";
      s3_stats_inc("authorization_failed_invalid_accesskey_count");
      s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
             error_code.c_str());
      request->respond_error(error_code);
    }
    done();
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  }
}
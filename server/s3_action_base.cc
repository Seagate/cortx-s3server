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

#include "s3_action_base.h"
#include "s3_motr_layout.h"
#include "s3_error_codes.h"
#include "s3_option.h"
#include "s3_stats.h"

S3Action::S3Action(std::shared_ptr<S3RequestObject> req, bool check_shutdown,
                   std::shared_ptr<S3AuthClientFactory> auth_factory,
                   bool skip_auth, bool skip_authorize)
    : Action(req, check_shutdown, auth_factory, skip_auth),
      request(req),
      skip_authorization(skip_authorize) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  setup_steps();
}

S3Action::~S3Action() { s3_log(S3_LOG_DEBUG, request_id, "Destructor\n"); }

void S3Action::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "S3Option::is_auth_disabled: (%d), skip_auth: (%d)\n",
         S3Option::get_instance()->is_auth_disabled(), skip_auth);
  ACTION_TASK_ADD(S3Action::load_metadata, this);
  if ((!S3Option::get_instance()->is_auth_disabled() && !skip_auth) &&
      (!skip_authorization)) {
    // add_task(std::bind( &S3Action::fetch_acl_policies, this ));
    // Commented till we implement Authorization feature completely.
    // Current authorisation implementation in AuthServer is partial
    ACTION_TASK_ADD(S3Action::set_authorization_meta, this);
    ACTION_TASK_ADD(S3Action::check_authorization, this);
  }
}

void S3Action::load_metadata() { next(); }
void S3Action::set_authorization_meta() { next(); }

void S3Action::check_authorization() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->check_authorization(
      std::bind(&S3Action::check_authorization_successful, this),
      std::bind(&S3Action::check_authorization_failed, this));
}

void S3Action::check_authorization_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3Action::check_authorization_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (request->client_connected()) {
    std::string error_code = auth_client->get_error_code();
    if (error_code == "InvalidAccessKeyId") {
      s3_stats_inc("authorization_failed_invalid_accesskey_count");
    } else if (error_code == "SignatureDoesNotMatch") {
      s3_stats_inc("authorization_failed_signature_mismatch_count");
    } else if (error_code == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "2");
    } else {
      // Possible error_code values: AccessDenied, MethodNotAllowed.
      // AccessDenied: When the requesting identity does not have
      // 'PutBucketPolicy' permission.
      //
      // MethodNotAllowed: When the requesting identity has required
      // permissions but does not belong to the bucket owner's account.
      // Refer AWS s3 for more details.
      set_s3_error(error_code);
    }
    s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
           error_code.c_str());
    request->respond_error(error_code);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

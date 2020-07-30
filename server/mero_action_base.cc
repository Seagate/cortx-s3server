#include "mero_action_base.h"
#include "s3_error_codes.h"
#include "s3_option.h"
#include "s3_stats.h"

MeroAction::MeroAction(std::shared_ptr<MeroRequestObject> req,
                       bool check_shutdown,
                       std::shared_ptr<S3AuthClientFactory> auth_factory,
                       bool skip_auth)
    : Action(req, check_shutdown, auth_factory, skip_auth), request(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  auth_client->do_skip_authorization();
  setup_steps();
}

MeroAction::~MeroAction() { s3_log(S3_LOG_DEBUG, request_id, "Destructor\n"); }

void MeroAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "S3Option::is_auth_disabled: (%d), skip_auth: (%d)\n",
         S3Option::get_instance()->is_auth_disabled(), skip_auth);

  if (!S3Option::get_instance()->is_auth_disabled() && !skip_auth) {
    ACTION_TASK_ADD(MeroAction::check_authorization, this);
  }
}

void MeroAction::check_authorization() {
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
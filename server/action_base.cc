/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-June-2019
 */

#include "action_base.h"
#include "s3_clovis_layout.h"
#include "s3_error_codes.h"
#include "s3_option.h"
#include "s3_stats.h"

Action::Action(std::shared_ptr<RequestObject> req, bool check_shutdown,
               std::shared_ptr<S3AuthClientFactory> auth_factory,
               bool skip_auth)
    : base_request(req),
      check_shutdown_signal(check_shutdown),
      is_response_scheduled(false),
      is_fi_hit(false),
      invalid_request(false),
      skip_auth(skip_auth) {
  request_id = base_request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  task_iteration_index = 0;
  rollback_index = 0;
  s3_error_code = "";
  state = ActionState::start;
  rollback_state = ActionState::start;
  mem_profile.reset(new S3MemoryProfile());
  // Set the callback that will be called, when read timeout happens
  base_request->set_client_read_timeout_callback(
      std::bind(&Action::client_read_timeout_callback, this));
  if (auth_factory) {
    auth_client_factory = auth_factory;
  } else {
    auth_client_factory = std::make_shared<S3AuthClientFactory>();
  }
  auth_client = auth_client_factory->create_auth_client(req);
  setup_steps();
}

Action::~Action() { s3_log(S3_LOG_DEBUG, request_id, "Destructor\n"); }

void Action::set_s3_error(std::string code) {
  state = ActionState::error;
  s3_error_code = code;
}

void Action::client_read_timeout_callback() {
  set_s3_error("RequestTimeout");
  if (rollback_state == ActionState::start) {
    rollback_start();
  } else {
    send_response_to_s3_client();
  }
  return;
}

std::string& Action::get_s3_error_code() { return s3_error_code; }

bool Action::is_error_state() { return state == ActionState::error; }

void Action::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "S3Option::is_auth_disabled: (%d), skip_auth: (%d)\n",
         S3Option::get_instance()->is_auth_disabled(), skip_auth);

  if (!S3Option::get_instance()->is_auth_disabled() && !skip_auth) {
    add_task(std::bind(&Action::check_authentication, this));
  }
}

void Action::start() {

  if (check_shutdown_signal && check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }

  task_iteration_index = 0;
  if (task_list.size() > 0) {
    state = ActionState::running;
    task_list[task_iteration_index++]();
  }
}

// Step to next async step.
void Action::next() {
  if ((check_shutdown_signal && check_shutdown_and_rollback()) ||
      base_request->is_s3_client_read_timedout()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (task_iteration_index < task_list.size()) {
    if (base_request->client_connected()) {
      task_list[task_iteration_index++]();
    } else {
      rollback_start();
    }
  } else {
    done();
  }
}

void Action::done() {
  task_iteration_index = 0;
  state = ActionState::complete;
}

void Action::pause() {
  // Set state as Paused.
  state = ActionState::paused;
}

void Action::resume() {
  // Resume only if paused.
  state = ActionState::running;
}

void Action::abort() {
  // Mark state as Aborted.
  task_iteration_index = 0;
  state = ActionState::stopped;
}

// rollback async steps
void Action::rollback_start() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  rollback_index = 0;
  rollback_state = ActionState::running;
  if (rollback_list.size())
    rollback_list[rollback_index++]();
  else {
    s3_log(S3_LOG_ERROR, request_id, "Rollback triggered on empty list\n");
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void Action::rollback_next() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (base_request->client_connected() &&
      rollback_index < rollback_list.size()) {
    // Call step and move index to next
    rollback_list[rollback_index++]();
  } else {
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void Action::rollback_done() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  rollback_index = 0;
  rollback_state = ActionState::complete;
  rollback_exit();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void Action::rollback_exit() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void Action::check_authentication() {
  auth_timer.start();
  auth_client->check_authentication(
      std::bind(&Action::check_authentication_successful, this),
      std::bind(&Action::check_authentication_failed, this));
}

void Action::check_authentication_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  auth_timer.stop();
  const auto mss = auth_timer.elapsed_time_in_millisec();
  LOG_PERF("check_authentication_ms", mss);
  s3_stats_timing("check_authentication", mss);

  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void Action::check_authentication_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (base_request->client_connected()) {
    std::string error_code = auth_client->get_error_code();
    if (error_code == "InvalidAccessKeyId") {
      s3_stats_inc("authentication_failed_invalid_accesskey_count");
    } else if (error_code == "SignatureDoesNotMatch") {
      s3_stats_inc("authentication_failed_signature_mismatch_count");
    }
    s3_log(S3_LOG_ERROR, request_id, "Authentication failure: %s\n",
           error_code.c_str());
    base_request->respond_error(error_code);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();
}

bool Action::check_shutdown_and_rollback(bool check_auth_op_aborted) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bool is_s3_shutting_down =
      S3Option::get_instance()->get_is_s3_shutting_down();
  if (is_s3_shutting_down && check_auth_op_aborted &&
      get_auth_client()->is_chunk_auth_op_aborted()) {
    if (get_rollback_state() == ActionState::start) {
      rollback_start();
    } else {
      send_response_to_s3_client();
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return is_s3_shutting_down;
  }
  if (!is_response_scheduled && is_s3_shutting_down) {
    s3_log(S3_LOG_DEBUG, request_id, "S3 server is about to shutdown\n");
    base_request->pause();
    is_response_scheduled = true;
    if (s3_error_code.empty()) {
      set_s3_error("ServiceUnavailable");
    }
    if (number_of_rollback_tasks()) {
      rollback_start();
    } else {
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return is_s3_shutting_down;
}

void Action::send_retry_error_to_s3_client(int retry_after_in_secs) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  base_request->respond_retry_after(1);
  done();
  i_am_done();
}

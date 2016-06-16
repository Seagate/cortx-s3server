/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original creation date: 1-Oct-2015
 */

#include "s3_action_base.h"
#include "s3_error_codes.h"
#include "s3_option.h"

S3Action::S3Action(std::shared_ptr<S3RequestObject> req) : request(req), invalid_request(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  task_iteration_index = 0;
  rollback_index = 0;
  error_message = "";
  state = S3ActionState::start;
  rollback_state = S3ActionState::start;
  setup_steps();
}

S3Action::~S3Action() { s3_log(S3_LOG_DEBUG, "Destructor\n"); }

void S3Action::get_error_message(std::string& message) {
    error_message = message;
}

void S3Action::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setup the action\n");

  if (!S3Option::get_instance()->is_auth_disabled()) {
    add_task(std::bind( &S3Action::check_authentication, this ));
    add_task(std::bind( &S3Action::fetch_acl_policies, this ));
    add_task(std::bind( &S3Action::check_authorization, this ));
  }
}

void S3Action::start() {
  task_iteration_index = 0;
  state = S3ActionState::running;
  task_list[task_iteration_index++]();
}
// Step to next async step.
void S3Action::next() {
  resume();
  if (task_iteration_index < task_list.size()) {
    if (request->client_connected()) {
      task_list[task_iteration_index++]();
    } else {
      i_am_done();
    }
  } else {
    done();
  }
}

void S3Action::done() {
  task_iteration_index = 0;
  state = S3ActionState::complete;
}

void S3Action::pause() {
  // Set state as Paused.
  state = S3ActionState::paused;
}

void S3Action::resume() {
  // Resume only if paused.
  state = S3ActionState::running;
}

void S3Action::abort() {
  // Mark state as Aborted.
  task_iteration_index = 0;
  state = S3ActionState::stopped;
}

// rollback async steps
void S3Action::rollback_start() {
  rollback_index = 0;
  rollback_state = S3ActionState::running;
  if (rollback_list.size())
    rollback_list[rollback_index++]();
  else {
    s3_log(S3_LOG_ERROR, "Rollback triggered on empty list\n");
    rollback_done();
  }
}

void S3Action::rollback_next() {
  if (rollback_index < rollback_list.size()) {
      // Call step and move index to next
      rollback_list[rollback_index++]();
  } else {
    rollback_done();
  }
}

void S3Action::rollback_done() {
  rollback_index = 0;
  rollback_state = S3ActionState::complete;
  rollback_exit();
}

void S3Action::rollback_exit() {
  send_response_to_s3_client();
}

void S3Action::fetch_acl_policies() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (request->get_api_type() == S3ApiType::object) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request);
    object_metadata->load(std::bind( &S3Action::next, this), std::bind( &S3Action::fetch_acl_object_policies_failed, this));
  } else if (request->get_api_type() == S3ApiType::bucket) {
    bucket_metadata = std::make_shared<S3BucketMetadata>(request);
    bucket_metadata->load(std::bind( &S3Action::next, this), std::bind( &S3Action::fetch_acl_bucket_policies_failed, this));
  } else {
    next();
  }
}

void S3Action::fetch_acl_object_policies_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_metadata->get_state() != S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_ERROR, "Authorization failure: failed to load acl/policies from object\n");
    request->send_response(S3HttpFailed400);
    done();
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    i_am_done();
  } else {
    next();
  }
}

void S3Action::fetch_acl_bucket_policies_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() != S3BucketMetadataState::missing) {
    s3_log(S3_LOG_ERROR, "Authorization failure: failed to load acl/policies from bucket\n");
    request->send_response(S3HttpFailed400);
    done();
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    i_am_done();
  } else {
    next();
  }
}

void S3Action::check_authorization() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (request->get_api_type() == S3ApiType::bucket) {
    auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(), bucket_metadata->get_policy_as_json());
  } else if (request->get_api_type() == S3ApiType::object) {
    auth_client->set_acl_and_policy(object_metadata->get_encoded_object_acl(), "");
  }
  auth_client->check_authorization(std::bind( &S3Action::check_authorization_successful, this), std::bind( &S3Action::check_authorization_failed, this));
}

void S3Action::check_authorization_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3Action::check_authorization_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (request->client_connected()) {
    std::string error_code = auth_client->get_error_code();
    s3_log(S3_LOG_ERROR, "Authorization failure: %s\n", error_code.c_str());

    S3Error error(error_code, request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
    s3_log(S3_LOG_ERROR, "Authorization failure (http status): %d\n", error.get_http_status_code());
    s3_log(S3_LOG_ERROR, "Authorization failure: %s\n", response_xml.c_str());
  }
  done();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  i_am_done();
}

void S3Action::check_authentication() {
  auth_client = std::make_shared<S3AuthClient>(request);
  auth_client->check_authentication(std::bind( &S3Action::check_authentication_successful, this), std::bind( &S3Action::check_authentication_failed, this));
}

void S3Action::check_authentication_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3Action::check_authentication_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (request->client_connected()) {
    std::string error_code = auth_client->get_error_code();
    s3_log(S3_LOG_ERROR, "Authentication failure: %s\n", error_code.c_str());

    S3Error error(error_code, request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
    s3_log(S3_LOG_ERROR, "Authentication failure (http status): %d\n", error.get_http_status_code());
    s3_log(S3_LOG_ERROR, "Authentication failure: %s\n", response_xml.c_str());
  }
  done();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  i_am_done();
}

void S3Action::start_chunk_authentication() {
  auth_client = std::make_shared<S3AuthClient>(request);
  auth_client->check_chunk_auth(std::bind( &S3Action::check_authentication_successful, this), std::bind( &S3Action::check_authentication_failed, this));
}

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
#include "s3_clovis_layout.h"
#include "s3_error_codes.h"
#include "s3_option.h"
#include "s3_stats.h"

S3Action::S3Action(std::shared_ptr<S3RequestObject> req, bool check_shutdown,
                   std::shared_ptr<S3AuthClientFactory> auth_factory,
                   bool skip_auth)
    : request(req),
      invalid_request(false),
      check_shutdown_signal(check_shutdown),
      skip_auth(skip_auth),
      is_response_scheduled(false),
      is_fi_hit(false) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  task_iteration_index = 0;
  rollback_index = 0;
  s3_error_code = "";
  state = S3ActionState::start;
  rollback_state = S3ActionState::start;
  mem_profile.reset(new S3MemoryProfile());
  if (auth_factory) {
    auth_client_factory = auth_factory;
  } else {
    auth_client_factory = std::make_shared<S3AuthClientFactory>();
  }
  auth_client = auth_client_factory->create_auth_client(req);
  setup_steps();
}

S3Action::~S3Action() { s3_log(S3_LOG_DEBUG, request_id, "Destructor\n"); }

void S3Action::set_s3_error(std::string code) {
  state = S3ActionState::error;
  s3_error_code = code;
}

std::string& S3Action::get_s3_error_code() { return s3_error_code; }

bool S3Action::is_error_state() { return state == S3ActionState::error; }

void S3Action::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "S3Option::is_auth_disabled: (%d), skip_auth: (%d)\n",
         S3Option::get_instance()->is_auth_disabled(), skip_auth);

  if (!S3Option::get_instance()->is_auth_disabled() && !skip_auth) {
    add_task(std::bind(&S3Action::check_authentication, this));
    // add_task(std::bind( &S3Action::fetch_acl_policies, this ));
    // Commented till we implement Authorization feature completely.
    // Current authorisation implementation in AuthServer is partial
    // add_task(std::bind( &S3Action::check_authorization, this ));
  }
}

void S3Action::start() {
  // Check if we have enough approx memory to proceed with request
  if (request->get_api_type() == S3ApiType::object) {
    int layout_id =
        S3ClovisLayoutMap::get_instance()->get_layout_for_object_size(
            request->get_data_length());
    if (request->http_verb() == S3HttpVerb::PUT &&
        !mem_profile->we_have_enough_memory_for_put_obj(layout_id)) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Limited memory: Rejecting PUT object/part request with retry.\n");
      send_retry_error_to_s3_client();
      return;
    }
  }

  if (check_shutdown_signal && check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }

  task_iteration_index = 0;
  if (task_list.size() > 0) {
    state = S3ActionState::running;
    task_list[task_iteration_index++]();
  }
}

// Step to next async step.
void S3Action::next() {
  if (check_shutdown_signal && check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
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
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  rollback_index = 0;
  rollback_state = S3ActionState::running;
  if (rollback_list.size())
    rollback_list[rollback_index++]();
  else {
    s3_log(S3_LOG_ERROR, request_id, "Rollback triggered on empty list\n");
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3Action::rollback_next() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (rollback_index < rollback_list.size()) {
    // Call step and move index to next
    rollback_list[rollback_index++]();
  } else {
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3Action::rollback_done() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  rollback_index = 0;
  rollback_state = S3ActionState::complete;
  rollback_exit();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3Action::rollback_exit() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// TODO -- When this function is enabled, for object we need to
// first fetch bucket details and then provide object list index oid to the
// constructor
// of S3Objectmetadata else load() will result in crash
//
// void S3Action::fetch_acl_policies() {
//  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
//  if (request->get_api_type() == S3ApiType::object) {
//    object_metadata = std::make_shared<S3ObjectMetadata>(request);
//    object_metadata->load(std::bind( &S3Action::next, this), std::bind(
//    &S3Action::fetch_acl_object_policies_failed, this));
//  } else if (request->get_api_type() == S3ApiType::bucket) {
//    bucket_metadata = std::make_shared<S3BucketMetadata>(request);
//    bucket_metadata->load(std::bind( &S3Action::next, this), std::bind(
//    &S3Action::fetch_acl_bucket_policies_failed, this));
//  } else {
//    next();
//  }
//}

// void S3Action::fetch_acl_object_policies_failed() {
//  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
//  if (object_metadata->get_state() != S3ObjectMetadataState::missing) {
//    s3_log(S3_LOG_ERROR, request_id, "Metadata lookup error: failed to load
// acl/policies
//    from object\n");
//    S3Error error("InternalError", request->get_request_id(),
//    request->get_object_uri());
//    std::string& response_xml = error.to_xml();
//    request->set_out_header_value("Content-Type", "application/xml");
//    request->set_out_header_value("Content-Length",
//    std::to_string(response_xml.length()));
//    request->send_response(error.get_http_status_code(), response_xml);

//    done();
//    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
//    i_am_done();
//  } else {
//    next();
//  }
//}

// void S3Action::fetch_acl_bucket_policies_failed() {
//  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
//  if (bucket_metadata->get_state() != S3BucketMetadataState::missing) {
//    s3_log(S3_LOG_ERROR, request_id, "Metadata lookup error: failed to load
// acl/policies
//    from bucket\n");
//    S3Error error("InternalError", request->get_request_id(),
//    request->get_object_uri());
//    std::string& response_xml = error.to_xml();
//    request->set_out_header_value("Content-Type", "application/xml");
//    request->set_out_header_value("Content-Length",
//    std::to_string(response_xml.length()));
//    request->send_response(error.get_http_status_code(), response_xml);

//    done();
//    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
//    i_am_done();
//  } else {
//    next();
//  }
//}

void S3Action::check_authorization() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (request->get_api_type() == S3ApiType::bucket) {
    auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                    bucket_metadata->get_policy_as_json());
  } else if (request->get_api_type() == S3ApiType::object) {
    auth_client->set_acl_and_policy(object_metadata->get_encoded_object_acl(),
                                    "");
  }
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
    }
    s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
           error_code.c_str());
    request->respond_error(error_code);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();
}

void S3Action::check_authentication() {
  auth_client->check_authentication(
      std::bind(&S3Action::check_authentication_successful, this),
      std::bind(&S3Action::check_authentication_failed, this));
}

void S3Action::check_authentication_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3Action::check_authentication_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (request->client_connected()) {
    std::string error_code = auth_client->get_error_code();
    if (error_code == "InvalidAccessKeyId") {
      s3_stats_inc("authentication_failed_invalid_accesskey_count");
    } else if (error_code == "SignatureDoesNotMatch") {
      s3_stats_inc("authentication_failed_signature_mismatch_count");
    }
    s3_log(S3_LOG_ERROR, request_id, "Authentication failure: %s\n",
           error_code.c_str());
    request->respond_error(error_code);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();
}

void S3Action::start_chunk_authentication() {
  auth_client->check_chunk_auth(
      std::bind(&S3Action::check_authentication_successful, this),
      std::bind(&S3Action::check_authentication_failed, this));
}

bool S3Action::check_shutdown_and_rollback(bool check_auth_op_aborted) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bool is_s3_shutting_down =
      S3Option::get_instance()->get_is_s3_shutting_down();
  if (is_s3_shutting_down && check_auth_op_aborted &&
      get_auth_client()->is_chunk_auth_op_aborted()) {
    if (get_rollback_state() == S3ActionState::start) {
      rollback_start();
    } else {
      send_response_to_s3_client();
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return is_s3_shutting_down;
  }
  if (!is_response_scheduled && is_s3_shutting_down) {
    s3_log(S3_LOG_DEBUG, request_id, "S3 server is about to shutdown\n");
    request->pause();
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

void S3Action::send_retry_error_to_s3_client(int retry_after_in_secs) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  request->respond_retry_after(1);
  done();
  i_am_done();
}

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

#pragma once

#ifndef __S3_SERVER_S3_AUTH_CLIENT_H__
#define __S3_SERVER_S3_AUTH_CLIENT_H__

#include <gtest/gtest_prod.h>
#include <functional>
#include <memory>
#include <queue>
#include <string>

#include "s3_asyncop_context_base.h"
#include "s3_auth_context.h"
#include "s3_auth_response_error.h"
#include "s3_auth_response_success.h"
#include "s3_log.h"
#include "s3_option.h"
#include "request_object.h"
#include "s3_addb.h"

// Should be used inside S3AuthClientOpContext and S3AuthClient only
#define ADDB_AUTH(req_state) \
  ADDB(S3_ADDB_AUTH_ID, request->addb_request_id, (req_state))

extern "C" evhtp_res on_auth_response(evhtp_request_t* req, evbuf_t* buf,
                                      void* arg);
extern "C" void on_event_hook(evhtp_connection_t* conn, short events,
                              void* arg);
extern "C" void on_write_hook(evhtp_connection_t* conn, void* arg);
extern "C" evhtp_res on_request_headers(evhtp_request_t* r, void* arg);
extern "C" evhtp_res on_conn_err_callback(evhtp_connection_t* connection,
                                          evhtp_error_flags errtype, void* arg);

class S3AuthClientOpContext : public S3AsyncOpContextBase {
  struct s3_auth_op_context* auth_op_context;

  bool is_auth_successful;
  bool is_authorization_successful;
  bool is_aclvalidation_successful;
  bool is_policyvalidation_successful;

  std::unique_ptr<S3AuthResponseSuccess> success_obj;
  std::unique_ptr<S3AuthResponseError> error_obj;

  std::string auth_response_xml;
  std::string authorization_response_xml;

 public:
  S3AuthClientOpContext(std::shared_ptr<RequestObject> req,
                        std::function<void()> success_callback,
                        std::function<void()> failed_callback)
      : S3AsyncOpContextBase(req, success_callback, failed_callback),
        auth_op_context(NULL),
        is_auth_successful(false),
        is_authorization_successful(false),
        is_aclvalidation_successful(false),
        is_policyvalidation_successful(false),
        auth_response_xml("") {
    s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
    ADDB_AUTH(ACTS_AUTH_OP_CTX_CONSTRUCT);
  }

  ~S3AuthClientOpContext() {
    s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");
    ADDB_AUTH(ACTS_AUTH_OP_CTX_DESTRUCT);
    clear_op_context();
  }

  void set_auth_response_error(std::string error_code,
                               std::string error_message,
                               std::string request_id);

  void set_auth_response_xml(const char* xml, bool success = true) {
    s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
    auth_response_xml = xml;
    is_auth_successful = success;
    if (is_auth_successful) {
      success_obj.reset(new S3AuthResponseSuccess(auth_response_xml));
      if (success_obj->isOK()) {
        get_request()->set_user_name(success_obj->get_user_name());
        get_request()->set_canonical_id(success_obj->get_canonical_id());
        get_request()->set_user_id(success_obj->get_user_id());
        get_request()->set_account_name(success_obj->get_account_name());
        get_request()->set_account_id(success_obj->get_account_id());
        get_request()->set_email(success_obj->get_email());
      } else {
        is_auth_successful = false;  // since auth response details are bad
      }
    } else {
      error_obj.reset(new S3AuthResponseError(auth_response_xml));
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  }

  void set_aclvalidation_response_xml(const char* xml, bool success = true) {
    s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
    is_aclvalidation_successful = success;
    auth_response_xml = xml;
    if (!success) {
      error_obj.reset(new S3AuthResponseError(auth_response_xml));
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  }

  void set_policyvalidation_response_xml(const char* xml, bool success = true) {
    s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
    is_policyvalidation_successful = success;
    auth_response_xml = xml;
    if (!success) {
      error_obj.reset(new S3AuthResponseError(auth_response_xml));
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  }

  void set_authorization_response(const char* xml, bool success = true) {
    s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
    authorization_response_xml = xml;
    is_authorization_successful = success;
    if (is_authorization_successful) {
      success_obj.reset(new S3AuthResponseSuccess(authorization_response_xml));
      if (success_obj->isOK()) {
        std::shared_ptr<S3RequestObject> s3_request =
            std::dynamic_pointer_cast<S3RequestObject>(request);
        if (s3_request != nullptr) {
          s3_request->set_user_name(success_obj->get_user_name());
          s3_request->set_canonical_id(success_obj->get_canonical_id());
          s3_request->set_user_id(success_obj->get_user_id());
          s3_request->set_account_name(success_obj->get_account_name());
          s3_request->set_account_id(success_obj->get_account_id());
          s3_request->set_default_acl(success_obj->get_acl());
          s3_request->set_email(success_obj->get_email());
        }
      } else {
        // Invalid authorisation response xml
        is_authorization_successful = false;
      }
    } else {
      error_obj.reset(new S3AuthResponseError(authorization_response_xml));
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  }

  bool auth_successful() { return is_auth_successful; }

  bool authorization_successful() { return is_authorization_successful; }

  bool aclvalidation_successful() { return is_aclvalidation_successful; }

  bool policyvalidation_successful() { return is_policyvalidation_successful; }

  std::string get_user_id() {
    if (is_auth_successful) {
      return success_obj->get_user_id();
    }
    return "";
  }

  std::string get_user_name() {
    if (is_auth_successful) {
      return success_obj->get_user_name();
    }
    return "";
  }

  std::string get_canonical_id() {
    if (is_auth_successful) {
      return success_obj->get_canonical_id();
    }
    return "";
  }

  std::string get_email() {
    if (is_auth_successful) {
      return success_obj->get_email();
    }
    return "";
  }

  std::string get_account_id() {
    if (is_auth_successful) {
      return success_obj->get_account_id();
    }
    return "";
  }

  std::string get_account_name() {
    if (is_auth_successful) {
      return success_obj->get_account_name();
    }
    return "";
  }

  std::string get_signature_sha256() {
    if (is_auth_successful) {
      return success_obj->get_signature_sha256();
    }
    return "";
  }

  const std::string& get_error_code() { return error_obj->get_code(); }

  const std::string& get_error_message() { return error_obj->get_message(); }

  // Call this when you want to do auth op.
  virtual bool init_auth_op_ctx(const S3AuthClientOpType& type) {
    struct event_base* eventbase = S3Option::get_instance()->get_eventbase();
    if (eventbase == NULL) {
      return false;
    }

    ADDB_AUTH(ACTS_AUTH_OP_CTX_NEW_CONN);
    clear_op_context();

    auth_op_context = create_basic_auth_op_ctx(eventbase);
    if (auth_op_context == NULL) {
      return false;
    }

    evhtp_set_hook(&auth_op_context->conn->hooks, evhtp_hook_on_event,
                   (evhtp_hook)on_event_hook, (void*)this);
    evhtp_set_hook(&auth_op_context->conn->hooks, evhtp_hook_on_write,
                   (evhtp_hook)on_write_hook, (void*)this);
    evhtp_set_hook(&auth_op_context->auth_request->hooks,
                   evhtp_hook_on_headers_start, (evhtp_hook)on_request_headers,
                   (void*)this);
    evhtp_set_hook(&auth_op_context->conn->hooks, evhtp_hook_on_conn_error,
                   (evhtp_hook)on_conn_err_callback, (void*)this);

    return true;
  }

  void unset_hooks() {
    if (auth_op_context == NULL) {
      return;
    }
    if ((auth_op_context->conn != NULL)) {
      evhtp_unset_all_hooks(&auth_op_context->conn->hooks);
    }
    if (auth_op_context->auth_request != NULL) {
      evhtp_unset_all_hooks(&auth_op_context->auth_request->hooks);
    }
  }

  void clear_op_context() {
    free_basic_auth_client_op_ctx(auth_op_context);
    auth_op_context = NULL;
  }

  struct s3_auth_op_context* get_auth_op_ctx() { return auth_op_context; }

  void set_auth_callbacks(std::function<void(void)> success,
                          std::function<void(void)> failed) {
    reset_callbacks(success, failed);
  }

  FRIEND_TEST(S3AuthClientOpContextTest, Constructor);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest,
              CanHandleParseErrorInAuthSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest,
              CanHandleParseErrorInAuthErrorResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthErrorResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthInvalidTokenErrorResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthorizationSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest,
              CanHandleParseErrorInAuthorizeSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthorizationErrorResponse);
  FRIEND_TEST(S3AuthClientOpContextTest,
              CanHandleParseErrorInAuthorizeErrorResponse);

  // For UT.
 private:
  bool error_resp_is_OK() {
    if (!is_auth_successful) {
      return error_obj->isOK();
    }
    return false;
  }

  bool success_resp_is_OK() {
    if (is_auth_successful) {
      return success_obj->isOK();
    }
    return false;
  }
};

enum class S3AuthClientOpState {
  init,
  started,
  authenticated,
  unauthenticated,
  authorized,
  unauthorized,
  connection_error
};

class S3AuthClient {
 private:
  std::shared_ptr<RequestObject> request;
  std::unique_ptr<S3AuthClientOpContext> auth_context;
  std::string request_id;
  std::string bucket_acl;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3AuthClientOpState state;
  S3AuthClientOpType op_type;

  short retry_count;

  // Key=val pairs to be sent in auth request body
  std::map<std::string, std::string> data_key_val;

  struct evbuffer* req_body_buffer;

  // Holds tuple (signature-received-in-current-chunk, sha256-chunk-data)
  // indexed in chunk order
  std::queue<std::tuple<std::string, std::string>> chunk_validation_data;
  bool is_chunked_auth;
  std::string prev_chunk_signature_from_auth;     // remember on each hop
  std::string current_chunk_signature_from_auth;  // remember on each hop
  std::string hash_sha256_current_chunk;
  std::string policy_str;
  std::string acl_str;
  std::string user_acl;
  bool last_chunk_added;
  bool is_authheader_present;
  bool chunk_auth_aborted;
  bool skip_authorization;
  void trigger_authentication(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed);
  void trigger_authentication();
  void remember_auth_details_in_request();

 public:
  S3AuthClient(std::shared_ptr<RequestObject> req,
               bool skip_authorization = false);

  virtual ~S3AuthClient() {
    s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");
    ADDB_AUTH(ACTS_AUTH_CLNT_DESTRUCT);
    if (state == S3AuthClientOpState::started && auth_context) {
      auth_context->unset_hooks();
    }
  }

  std::shared_ptr<RequestObject> get_request() { return request; }
  std::string get_request_id() { return request_id; }

  S3AuthClientOpState get_state() { return state; }

  S3AuthClientOpType get_op_type() { return op_type; }

  void set_state(S3AuthClientOpState auth_state) { state = auth_state; }

  void set_op_type(S3AuthClientOpType type) { op_type = type; }

  // Returns AccessDenied | InvalidAccessKeyId | SignatureDoesNotMatch
  // auth InactiveAccessKey maps to InvalidAccessKeyId in S3
  std::string get_error_code();
  std::string get_error_message();

  std::string get_signature_from_response();

  void do_skip_authorization() { skip_authorization = true; }
  void setup_auth_request_headers();
  bool setup_auth_request_body();
  virtual void execute_authconnect_request(struct s3_auth_op_context* auth_ctx);
  void add_key_val_to_body(std::string key, std::string val);
  void set_is_authheader_present(bool is_authorizationheader_present);

  // Non-aws-chunk auth
  void check_authentication();
  void check_authentication(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed);
  void check_authentication_successful();
  void check_authentication_failed();

  void check_aclvalidation_successful();
  void check_aclvalidation_failed();

  void policy_validation_successful();
  void policy_validation_failed();

  void check_authorization();
  void validate_acl(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void validate_policy(std::function<void(void)> on_success,
                       std::function<void(void)> on_failed);
  void set_validate_acl(const std::string& validateacl);
  void check_authorization(std::function<void(void)> on_success,
                           std::function<void(void)> on_failed);
  void check_authorization_successful();
  void check_authorization_failed();

  // This is same as above but will cycle through with the
  // add_checksum_for_chunk() to validate each chunk we receive. Init =
  // headers auth, start = each chunk auth
  void check_chunk_auth(std::function<void(void)> on_success,
                        std::function<void(void)> on_failed);
  virtual void init_chunk_auth_cycle(std::function<void(void)> on_success,
                                     std::function<void(void)> on_failed);
  // Insert the signature sent in each chunk (chunk-signature=value)
  // and sha256(chunk-data) to be used in auth of chunk
  virtual void add_checksum_for_chunk(std::string current_sign,
                                      std::string sha256_of_payload);
  virtual void add_last_checksum_for_chunk(std::string current_sign,
                                           std::string sha256_of_payload);
  void chunk_auth_successful();
  void chunk_auth_failed();
  void abort_chunk_auth_op() { chunk_auth_aborted = true; }
  bool is_chunk_auth_op_aborted() { return chunk_auth_aborted; }
  void set_acl_and_policy(const std::string& acl, const std::string& policy);
  void set_bucket_acl(const std::string& bucket_acl);
  void set_event_with_retry_interval();

  FRIEND_TEST(S3AuthClientTest, Constructor);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyGet);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyHead);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyPut);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyPost);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyDelete);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyWithQueryParams);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyForChunkedAuth);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyForChunkedAuth1);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBodyForChunkedAuth2);
  FRIEND_TEST(S3BucketActionTest, SetAuthorizationMeta);
};

struct event_auth_timeout_arg {
  S3AuthClient* auth_client;
  struct event* event;
};

#endif

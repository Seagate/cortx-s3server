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

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include <gtest/gtest_prod.h>

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

evhtp_res on_read_response(evhtp_request_t* p_req, evbuf_t* p_buf, void* p_arg);

class S3AuthClientOpContext : public S3AsyncOpContextBase {

  std::string s_resp;

  std::unique_ptr<S3AuthResponseSuccess> success_obj;
  std::unique_ptr<S3AuthResponseError> error_obj;

  struct s3_auth_op_context* auth_op_context = NULL;
  long content_length = -1;
  bool f_success = false;

 public:
  const S3AuthClientOpType op_type;

  S3AuthClientOpContext(std::shared_ptr<RequestObject> req,
                        std::function<void()> success_callback,
                        std::function<void()> failed_callback,
                        S3AuthClientOpType op_type);
  ~S3AuthClientOpContext();

  const std::string& get_request_id() const { return request_id; }

  const std::string& get_stripped_request_id() const {
    return stripped_request_id;
  }

  long get_content_length() const { return content_length; }
  void save_content_length(long val) { content_length = val; }

  void set_auth_response_error(std::string error_code,
                               std::string error_message,
                               std::string request_id);

  const char* get_auth_status_message(int http_status) const noexcept;

  void handle_response(const char* sz_resp, int http_status);

  bool is_success() const { return f_success; }

  std::string get_signature_sha256() const {
    return f_success ? success_obj->get_signature_sha256() : "";
  }
  const std::string& get_error_message() const {
    return error_obj->get_message();
  }
  const std::string& get_error_code() const { return error_obj->get_code(); }

  // Call this when you want to do auth op.
  virtual bool init_auth_op_ctx();

  void unset_hooks();
  void clear_op_context();

  struct s3_auth_op_context* get_auth_op_ctx() const { return auth_op_context; }

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
 protected:
  bool error_resp_is_OK() const { return !f_success && error_obj->isOK(); }

  bool success_resp_is_OK() const { return f_success && success_obj->isOK(); }
};

enum class S3AuthClientOpState {
  connection_error = -2,
  failed = -1,
  init = 0,
  started,
  succeded
};

class S3AuthClient {

  std::shared_ptr<RequestObject> request;
  std::unique_ptr<S3AuthClientOpContext> auth_context;
  std::string request_id;
  std::string stripped_request_id;
  std::string bucket_acl;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3AuthClientOpState state = S3AuthClientOpState::init;
  S3AuthClientOpType op_type;

  unsigned retry_count;

  // Key=val pairs to be sent in auth request body
  std::map<std::string, std::string> data_key_val;

  struct evbuffer* req_body_buffer = nullptr;

  // Holds tuple (signature-received-in-current-chunk, sha256-chunk-data)
  // indexed in chunk order
  std::queue<std::pair<std::string, std::string>> chunk_validation_data;

  std::string prev_chunk_signature_from_auth;     // remember on each hop
  std::string current_chunk_signature_from_auth;  // remember on each hop
  std::string hash_sha256_current_chunk;
  std::string policy_str;
  std::string acl_str;
  std::string user_acl;
  std::string entity_path;

  bool last_chunk_added = false;
  bool is_chunked_auth = false;
  bool chunk_auth_aborted = false;
  bool is_authheader_present;

  bool skip_authorization;

  void trigger_request(std::function<void(void)> on_success,
                       std::function<void(void)> on_failed);
  // void remember_auth_details_in_request();

  void on_common_success();
  void on_common_failed();

 public:
  S3AuthClient(std::shared_ptr<RequestObject> req,
               bool skip_authorization = false);
  bool set_get_method = false;

  virtual ~S3AuthClient();

  std::shared_ptr<RequestObject> get_request() { return request; }
  S3AuthClientOpState get_state() const { return state; }
  S3AuthClientOpType get_op_type() const { return op_type; }

  void set_state(S3AuthClientOpState auth_state) { state = auth_state; }

  // Returns AccessDenied | InvalidAccessKeyId | SignatureDoesNotMatch
  // auth InactiveAccessKey maps to InvalidAccessKeyId in S3
  std::string get_error_code() const;
  std::string get_error_message() const;

  std::string get_signature_from_response();

  void do_skip_authorization() { skip_authorization = true; }

  void setup_auth_request_headers();
  bool setup_auth_request_body();

  virtual void execute_authconnect_request(struct s3_auth_op_context* auth_ctx);
  void add_key_val_to_body(std::string key, std::string val);
  void add_non_empty_key_val_to_body(std::string key, std::string val);
  void add_non_empty_key_val_to_body(std::string key,
                                     const std::list<std::string>& val_list);
  void set_is_authheader_present(bool is_authorizationheader_present);
  void trigger_request();

  // Non-aws-chunk auth
  void check_authentication(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed);
  void validate_acl(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void validate_policy(std::function<void(void)> on_success,
                       std::function<void(void)> on_failed);
  void set_validate_acl(const std::string& validateacl);

  void check_authorization(std::function<void(void)> on_success,
                           std::function<void(void)> on_failed);
  void check_combo_auth(std::function<void(void)> on_success,
                        std::function<void(void)> on_failed);

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

  void set_entity_path(std::string entity_path) {
    this->entity_path = std::move(entity_path);
  }

  friend class S3AuthClientTest;

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

#endif  // __S3_SERVER_S3_AUTH_CLIENT_H__

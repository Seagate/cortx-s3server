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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_AUTH_CLIENT_H__
#define __MERO_FE_S3_SERVER_S3_AUTH_CLIENT_H__

#include <memory>
#include <functional>
#include <queue>
#include <gtest/gtest_prod.h>

#include "s3_request_object.h"
#include "s3_auth_context.h"
#include "s3_asyncop_context_base.h"
#include "s3_auth_response_success.h"
#include "s3_auth_response_error.h"
#include "s3_log.h"

extern "C" evhtp_res on_auth_response(evhtp_request_t * req, evbuf_t * buf, void * arg);

class S3AuthClientOpContext : public S3AsyncOpContextBase {
  struct s3_auth_op_context* auth_op_context;
  bool has_auth_op_context;

  bool is_auth_successful;
  bool is_authorization_successful;
  S3AuthResponseSuccess *success_obj;
  S3AuthResponseError *error_obj;

  std::string auth_response_xml;
  std::string authorization_response;
public:
  S3AuthClientOpContext(std::shared_ptr<S3RequestObject> req,
                        std::function<void()> success_callback,
                        std::function<void()> failed_callback)
                      : S3AsyncOpContextBase(req, success_callback, failed_callback),
                        auth_op_context(NULL), has_auth_op_context(false),
                        is_auth_successful(false), is_authorization_successful(false), success_obj(NULL), error_obj(NULL),
                        auth_response_xml("") {
    s3_log(S3_LOG_DEBUG, "Constructor\n");
  }

  ~S3AuthClientOpContext() {
    s3_log(S3_LOG_DEBUG, "Destructor\n");
    if (has_auth_op_context) {
      free_basic_auth_client_op_ctx(auth_op_context);
      auth_op_context = NULL;
    }
    if (success_obj != NULL) {
      delete success_obj;
    }
    if (error_obj != NULL) {
      delete error_obj;
    }
  }

  void set_auth_response_xml(const char* xml, bool success = true) {
    s3_log(S3_LOG_DEBUG, "Entering\n");
    auth_response_xml = xml;
    is_auth_successful = success;
    if (is_auth_successful) {
      success_obj = new S3AuthResponseSuccess(auth_response_xml);
      if (success_obj->isOK()) {
        get_request()->set_user_name(success_obj->get_user_name());
        get_request()->set_user_id(success_obj->get_user_id());
        get_request()->set_account_name(success_obj->get_account_name());
        get_request()->set_account_id(success_obj->get_account_id());
      } else {
        is_auth_successful = false; // since auth response details are bad
      }
    } else {
      error_obj = new S3AuthResponseError(auth_response_xml);
    }
    s3_log(S3_LOG_DEBUG, "Exiting\n");
  }

  void set_authorization_response(const char *resp, bool success = true) {
    s3_log(S3_LOG_DEBUG, "Entering\n");
    authorization_response = resp;
    is_authorization_successful = success;
  }

  S3AuthResponseError* get_error_res_obj() {
    return error_obj;
  }

  S3AuthResponseSuccess* get_success_res_obj() {
    return success_obj;
  }

  // Call this when you want to do auth op.
  virtual bool init_auth_op_ctx() {
    struct event_base *eventbase = get_request()->get_evbase();
    if (eventbase == NULL) {
      return false;
    }
    auth_op_context = create_basic_auth_op_ctx(eventbase);
    if (auth_op_context == NULL) {
      return false;
    }
    has_auth_op_context = true;
    return true;
  }

  struct s3_auth_op_context* get_auth_op_ctx() {
    return auth_op_context;
  }

  void set_auth_callbacks(std::function<void(void)> success, std::function<void(void)> failed) {
    reset_callbacks(success, failed);
  }

  FRIEND_TEST(S3AuthClientOpContextTest, Constructor);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanHandleParseErrorInAuthSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthErrorResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthorizationSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanHandleParseErrorInAuthorizeSuccessResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanParseAuthorizationErrorResponse);
  FRIEND_TEST(S3AuthClientOpContextTest, CanHandleParseErrorInAuthorizeErrorResponse);
};

enum class S3AuthClientOpState {
  init,
  started,
  authenticated,
  unauthenticated,
  authorized,
  unauthorized
};

class S3AuthClient {
private:

  std::shared_ptr<S3RequestObject> request;
  std::unique_ptr<S3AuthClientOpContext> auth_context;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3AuthClientOpState state;

  // Key=val pairs to be sent in auth request body
  std::map<std::string, std::string> data_key_val;

  struct evbuffer* req_body_buffer;

  // Helper
  void setup_auth_request_headers(bool is_authorization_request);
  void setup_auth_request_body(bool is_authorization_request);
  virtual void execute_authconnect_request(struct s3_auth_op_context* auth_ctx, bool is_authorization_request);

  // Holds tuple (signature-received-in-current-chunk, sha256-chunk-data)
  // indexed in chunk order
  std::queue<std::tuple<std::string, std::string>> chunk_validation_data;
  bool is_chunked_auth;
  std::string prev_chunk_signature_from_auth;  // remember on each hop
  std::string current_chunk_signature_from_auth;  // remember on each hop
  std::string hash_sha256_current_chunk;
  std::string policy_str;
  std::string acl_str;
  bool last_chunk_added;

  void trigger_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void remember_auth_details_in_request();

public:
  S3AuthClient(std::shared_ptr<S3RequestObject> req);
  virtual ~S3AuthClient() {
    s3_log(S3_LOG_DEBUG, "Destructor\n");
  }

  S3AuthClientOpState get_state() {
    return state;
  }

  // Returns AccessDenied | InvalidAccessKeyId | SignatureDoesNotMatch
  // auth InactiveAccessKey maps to InvalidAccessKeyId in S3
  std::string get_error_code();
  std::string get_signature_from_response();

  void add_key_val_to_body(std::string key, std::string val);

  // Non-aws-chunk auth
  void check_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void check_authentication_successful();
  void check_authentication_failed();

  void check_authorization(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void check_authorization_successful();
  void check_authorization_failed();

  // This is same as above but will cycle through with the add_checksum_for_chunk()
  // to validate each chunk we receive. Init = headers auth, start = each chunk auth
  void check_chunk_auth(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void init_chunk_auth_cycle(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  // Insert the signature sent in each chunk (chunk-signature=value)
  // and sha256(chunk-data) to be used in auth of chunk
  void add_checksum_for_chunk(std::string current_sign, std::string sha256_of_payload);
  void add_last_checksum_for_chunk(std::string current_sign, std::string sha256_of_payload);
  void chunk_auth_successful();
  void chunk_auth_failed();
  void set_acl_and_policy(std::string acl, std::string policy);

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
};

#endif

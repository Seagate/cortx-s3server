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

#include <unistd.h>
#include <string>

#include "s3_common.h"
#include "s3_error_codes.h"
#include "s3_auth_client.h"
#include "s3_url_encode.h"

/* S3 Auth client callbacks */

extern "C" evhtp_res on_auth_conn_err_callback(evhtp_connection_t * connection, evhtp_error_flags errtype, void * arg) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  S3AuthClientOpContext *context = (S3AuthClientOpContext*)arg;

  // Note: Do not remove this, else you will have s3 crashes as the
  // callbacks are invoked after request/connection is freed.
  evhtp_unset_all_hooks(&context->get_auth_op_ctx()->conn->hooks);
  evhtp_unset_all_hooks(&context->get_auth_op_ctx()->authrequest->hooks);

  if (!context->get_request()->client_connected()) {
    // S3 client has already disconnected, ignore
    s3_log(S3_LOG_DEBUG, "S3 Client has already disconnected.\n");
    return EVHTP_RES_OK;
  }

  context->set_op_status_for(0, S3AsyncOpStatus::failed, "Cannot connect to Auth server.");
  context->set_auth_response_xml("", false);
  s3_log(S3_LOG_FATAL, "Cannot connect to Auth server.\n");

  context->on_failed_handler()();  // Invoke the handler.
  return EVHTP_RES_OK;
}

extern "C" evhtp_res on_auth_response(evhtp_request_t * req, evbuf_t * buf, void * arg) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  unsigned int auth_resp_status = evhtp_request_status(req);
  s3_log(S3_LOG_DEBUG, "auth_resp_status = %d\n", auth_resp_status);

  S3AuthClientOpContext *context = (S3AuthClientOpContext*)arg;

  // Note: Do not remove this, else you will have s3 crashes as the
  // callbacks are invoked after request/connection is freed.
  evhtp_unset_all_hooks(&context->get_auth_op_ctx()->conn->hooks);
  evhtp_unset_all_hooks(&context->get_auth_op_ctx()->authrequest->hooks);

  if (!context->get_request()->client_connected()) {
    // S3 client has already disconnected, ignore
    s3_log(S3_LOG_DEBUG, "S3 Client has already disconnected.\n");
    return EVHTP_RES_OK;
  }

  size_t buffer_len = evbuffer_get_length(buf) + 1;
  char *auth_response_body = (char*)malloc(buffer_len * sizeof(char));
  memset(auth_response_body, 0, buffer_len);
  evbuffer_copyout(buf, auth_response_body, buffer_len);
  s3_log(S3_LOG_DEBUG, "Response data received from Auth service = [[%s]]\n\n\n", auth_response_body);

  if (auth_resp_status == S3HttpSuccess200) {
    s3_log(S3_LOG_DEBUG, "Authentication successful\n");
    context->set_op_status_for(0, S3AsyncOpStatus::success, "Authentication successful");
    context->set_auth_response_xml(auth_response_body, true);
  } else if (auth_resp_status == S3HttpFailed401){
    s3_log(S3_LOG_ERROR, "Authentication failed\n");
    context->set_op_status_for(0, S3AsyncOpStatus::failed, "Authentication failed");
    context->set_auth_response_xml(auth_response_body, false);
  } else {
    s3_log(S3_LOG_FATAL, "Something is wrong with Auth server\n");
    context->set_op_status_for(0, S3AsyncOpStatus::failed, "Something is wrong with Auth server");
    context->set_auth_response_xml("", false);
  }
  free(auth_response_body);

  if (context->get_op_status_for(0) == S3AsyncOpStatus::success) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return EVHTP_RES_OK;
}

/******/

S3AuthClient::S3AuthClient(std::shared_ptr<S3RequestObject> req)
    : request(req), state(S3AuthClientOpState::init), is_chunked_auth(false), last_chunk_added(false) {
      s3_log(S3_LOG_DEBUG, "Constructor\n");
}

void S3AuthClient::add_key_val_to_body(std::string key, std::string val) {
  data_key_val[key] = val;
}

std::string S3AuthClient::get_signature_from_response() {
  if (auth_context->get_success_res_obj()) {
    return auth_context->get_success_res_obj()->get_signature_sha256();
  }
  return "";
}

void S3AuthClient::remember_auth_details_in_request() {
  if (auth_context->get_success_res_obj()) {
    request->set_user_id(auth_context->get_success_res_obj()->get_user_id());
    request->set_user_name(auth_context->get_success_res_obj()->get_user_name());
    request->set_account_id(auth_context->get_success_res_obj()->get_account_id());
    request->set_account_name(auth_context->get_success_res_obj()->get_account_name());
  }
}

// Returns AccessDenied | InvalidAccessKeyId | SignatureDoesNotMatch
// auth InactiveAccessKey maps to InvalidAccessKeyId in S3
std::string S3AuthClient::get_error_code() {
  if (auth_context->get_error_res_obj()) {
    std::string code = auth_context->get_error_res_obj()->get_code();
    if (code == "InactiveAccessKey") {
      return "InvalidAccessKeyId";
    } else if (code == "ExpiredCredential") {
      return "ExpiredToken";
    } else if (code == "InvalidClientTokenId") {
      return "InvalidToken";
    } else if (code == "TokenRefreshRequired") {
      return "AccessDenied";
    } else  if (!code.empty()){
      return code;  // InvalidAccessKeyId | SignatureDoesNotMatch
    }
  }
  return "ServiceUnavailable";  // auth server may be down
}

void S3AuthClient::setup_auth_request_body() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  std::string method = "";
  if (request->http_verb() == S3HttpVerb::GET) {
    method = "GET";
    // evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Method=GET&");
  } else if (request->http_verb() == S3HttpVerb::HEAD) {
    method = "HEAD";
  } else if (request->http_verb() == S3HttpVerb::PUT) {
    method = "PUT";
  } else if (request->http_verb() == S3HttpVerb::DELETE) {
    method = "DELETE";
  } else if (request->http_verb() == S3HttpVerb::POST) {
    method = "POST";
  }
  add_key_val_to_body("Method", method);


  const char *full_path = request->c_get_full_path();
  if (full_path != NULL) {
    add_key_val_to_body("ClientAbsoluteUri", full_path);
  } else {
    add_key_val_to_body("ClientAbsoluteUri", "");
  }

  const char *uri_query = request->c_get_uri_query();
  if (uri_query != NULL) {
    s3_log(S3_LOG_DEBUG, "c_get_uri_query = %s\n",uri_query);
    add_key_val_to_body("ClientQueryParams", uri_query);
  } else {
    add_key_val_to_body("ClientQueryParams", "");
  }

  // May need to take it from config
  add_key_val_to_body("Version", "2010-05-08");

  if (is_chunked_auth &&
      !( prev_chunk_signature_from_auth.empty()
        || current_chunk_signature_from_auth.empty() )
      ) {
    add_key_val_to_body("previous-signature-sha256", prev_chunk_signature_from_auth);
    add_key_val_to_body("current-signature-sha256", current_chunk_signature_from_auth);
    add_key_val_to_body("x-amz-content-sha256", hash_sha256_current_chunk);
  }

  // Lets form the body to be sent to Auth server.
  // struct s3_auth_op_context* op_ctx = auth_context->get_auth_op_ctx();
  std::string auth_request_body = "Action=AuthenticateUser";
  for (auto it: data_key_val) {
    auth_request_body += "&" + url_encode(it.first.c_str()) + "=" + url_encode(it.second.c_str());
  }

  s3_log(S3_LOG_DEBUG, "Request body sent to Auth server:\n%s\n", auth_request_body.c_str());
  // evbuffer_add_printf(op_ctx->authrequest->buffer_out, auth_request_body.c_str());
  req_body_buffer = evbuffer_new();
  evbuffer_add(req_body_buffer, auth_request_body.c_str(), auth_request_body.length());

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::setup_auth_request_headers() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  struct s3_auth_op_context* op_ctx = auth_context->get_auth_op_ctx();
  char sz_size[100] = {0};

  size_t out_len = evbuffer_get_length(req_body_buffer);
  sprintf(sz_size, "%zu", out_len);
  s3_log(S3_LOG_DEBUG, "Header - Length = %zu\n", out_len);
  s3_log(S3_LOG_DEBUG, "Header - Length-str = %s\n", sz_size);

  std::string host = request->get_host_header();
  if (!host.empty()) {
    evhtp_headers_add_header(op_ctx->authrequest->headers_out,
        evhtp_header_new("Host", host.c_str(), 1, 1));
  }
  evhtp_headers_add_header(op_ctx->authrequest->headers_out,
      evhtp_header_new("Content-Length", sz_size, 1, 1));
  evhtp_headers_add_header(op_ctx->authrequest->headers_out,
      evhtp_header_new("Content-Type", "application/x-www-form-urlencoded", 0, 0));
  evhtp_headers_add_header(op_ctx->authrequest->headers_out,
      evhtp_header_new("User-Agent", "s3server", 1, 1));
  evhtp_headers_add_header(op_ctx->authrequest->headers_out,
      evhtp_header_new("Connection", "close", 1, 1));

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::execute_authconnect_request(struct s3_auth_op_context* auth_ctx) {
  evhtp_make_request(auth_ctx->conn, auth_ctx->authrequest, htp_method_POST, "/");
  // evhtp_send_reply_body(auth_ctx->authrequest, auth_ctx->authrequest->buffer_out);
  evhtp_send_reply_body(auth_ctx->authrequest, req_body_buffer);
  evbuffer_free(req_body_buffer);
}

// Note: S3AuthClientOpContext should be created before calling this.
// This is called by check_authentication and start_chunk_auth
void S3AuthClient::trigger_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  state = S3AuthClientOpState::started;

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  auth_context->init_auth_op_ctx();
  struct s3_auth_op_context* auth_ctx = auth_context->get_auth_op_ctx();

  // Setup the auth callbacks
  evhtp_set_hook(&auth_ctx->authrequest->hooks, evhtp_hook_on_read, (evhtp_hook)on_auth_response, auth_context.get());
  evhtp_set_hook(&auth_ctx->conn->hooks, evhtp_hook_on_conn_error, (evhtp_hook)on_auth_conn_err_callback, auth_context.get());

  // Setup the headers to forward to auth service
  s3_log(S3_LOG_DEBUG, "Headers from S3 client:\n");
  for (auto it: request->get_in_headers_copy()) {
    s3_log(S3_LOG_DEBUG, "Header = %s, Value = %s\n", it.first.c_str(), it.second.c_str());
    add_key_val_to_body(it.first.c_str(), it.second.c_str());
  }

  // Setup the body to be sent to auth service
  setup_auth_request_body();
  setup_auth_request_headers();

  char mybuff[2000] = {0};
  evbuffer_copyout(req_body_buffer, mybuff, sizeof(mybuff));
  s3_log(S3_LOG_DEBUG, "Data being send to Auth server:\n%s\n", mybuff);

  execute_authconnect_request(auth_ctx);

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::check_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  auth_context.reset(new S3AuthClientOpContext(request, std::bind( &S3AuthClient::check_authentication_successful, this), std::bind( &S3AuthClient::check_authentication_failed, this)));

  is_chunked_auth = false;

  trigger_authentication(on_success, on_failed);
}

void S3AuthClient::check_authentication_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3AuthClientOpState::authenticated;
  remember_auth_details_in_request();
  prev_chunk_signature_from_auth = get_signature_from_response();
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::check_authentication_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Authentication failure\n");
  state = S3AuthClientOpState::unauthenticated;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

// This is same as above but will cycle through with the add_checksum_for_chunk()
// to validate each chunk we receive.
void S3AuthClient::check_chunk_auth(std::function<void(void)> on_success, std::function<void(void)> on_failed) {

  is_chunked_auth = true;

  check_authentication(on_success, on_failed);
}

// This is same as above but will cycle through with the add_checksum_for_chunk()
// to validate each chunk we receive.
void S3AuthClient::init_chunk_auth_cycle(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  auth_context.reset(new S3AuthClientOpContext(request, std::bind( &S3AuthClient::chunk_auth_successful, this), std::bind( &S3AuthClient::chunk_auth_failed, this)));

  is_chunked_auth = true;

  // trigger is done when sign and hash are available for any chunk
}

// Insert the signature sent in each chunk (chunk-signature=value)
// and sha256(chunk-data) to be used in auth of chunk
void S3AuthClient::add_checksum_for_chunk(std::string current_sign, std::string sha256_of_payload) {
  chunk_validation_data.push(std::make_tuple(current_sign, sha256_of_payload));
  if (state != S3AuthClientOpState::started) {
    current_chunk_signature_from_auth = std::get<0>(chunk_validation_data.front());
    hash_sha256_current_chunk = std::get<1>(chunk_validation_data.front());
    chunk_validation_data.pop();
    trigger_authentication(this->handler_on_success, this->handler_on_failed);
  }
}

void S3AuthClient::add_last_checksum_for_chunk(std::string current_sign, std::string sha256_of_payload) {
  last_chunk_added = true;
  add_checksum_for_chunk(current_sign, sha256_of_payload);
}

void S3AuthClient::chunk_auth_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3AuthClientOpState::authenticated;

  remember_auth_details_in_request();
  prev_chunk_signature_from_auth = get_signature_from_response();

  if (last_chunk_added && chunk_validation_data.empty()) {
    // we are done with all validations
    this->handler_on_success();
  } else {
    if (chunk_validation_data.empty()) {
      // we have to wait for more chunk signatures, which will be
      // added with add_checksum_for_chunk/add_last_checksum_for_chunk
    } else {
      // Validate next chunk signature
      current_chunk_signature_from_auth = std::get<0>(chunk_validation_data.front());
      hash_sha256_current_chunk = std::get<1>(chunk_validation_data.front());
      chunk_validation_data.pop();
      trigger_authentication(this->handler_on_success, this->handler_on_failed);
    }
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::chunk_auth_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Authentication failure\n");
  state = S3AuthClientOpState::unauthenticated;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

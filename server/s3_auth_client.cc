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

#include <event2/event.h>
#include <evhttp.h>
#include <unistd.h>
#include <string>

#include "s3_auth_client.h"
#include "s3_auth_fake.h"
#include "s3_common.h"
#include "s3_error_codes.h"
#include "s3_fi_common.h"
#include "s3_iem.h"
#include "s3_option.h"
#include "s3_common_utilities.h"
#include "atexit.h"

// S3 Auth client callbacks

static int log_http_header(evhtp_header_t *header, void *arg) {
  s3_log(S3_LOG_DEBUG, (char *)arg, "%s: %s", header->key, header->val);
  return 0;
}

static evhtp_res on_conn_err_callback(evhtp_connection_t *p_evhtp_conn,
                                      evhtp_error_flags errtype, void *p_arg) {
  assert(p_arg != nullptr);
  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);

  auto request_inst = p_auth_ctx->get_request();

  const auto &request_id = p_auth_ctx->get_request_id();
  const auto &stripped_request_id = p_auth_ctx->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  if (p_evhtp_conn == NULL) {
    s3_log(S3_LOG_ERROR, request_id, "input p_evhtp_conn is NULL\n");
    return EVHTP_RES_OK;
  }
  if (p_evhtp_conn != p_auth_ctx->get_auth_op_ctx()->conn) {
    s3_log(
        S3_LOG_ERROR, request_id,
        "Mismatch between input p_evhtp_conn and auth context p_evhtp_conn\n");
  }
  std::string errtype_str =
      S3CommonUtilities::evhtp_error_flags_description(errtype);
  if (p_evhtp_conn->request) {
    s3_log(S3_LOG_INFO, stripped_request_id, "Connection status = %d\n",
           p_evhtp_conn->request->status);

    if (p_evhtp_conn->request->headers_in) {
      s3_log(S3_LOG_DEBUG, request_id, "Headers IN:");
      evhtp_headers_for_each(p_evhtp_conn->request->headers_in, log_http_header,
                             (void *)request_id.c_str());
    }
  }
  s3_log(
      S3_LOG_WARN, request_id, "Socket error: %s, errno: %d, set errtype: %s\n",
      evutil_socket_error_to_string(evutil_socket_geterror(p_evhtp_conn->sock)),
      evutil_socket_geterror(p_evhtp_conn->sock), errtype_str.c_str());

  ADDB(S3_ADDB_AUTH_ID, request_inst->addb_request_id,
       ACTS_AUTH_OP_ON_CONN_ERR);

  // Note: Do not remove this, else you will have s3 crashes as the
  // callbacks are invoked after request/connection is freed.
  evhtp_unset_all_hooks(&p_evhtp_conn->hooks);

  if (p_evhtp_conn->request) {
    evhtp_unset_all_hooks(&p_evhtp_conn->request->hooks);
  }
  if (request_inst->client_connected()) {
    p_auth_ctx->set_op_status_for(0, S3AsyncOpStatus::connection_failed,
                                  "Cannot connect to Auth server.");
    p_auth_ctx->set_auth_response_error(
        "InternalError", "Cannot connect to Auth server", request_id);
  }
  p_auth_ctx->on_failed_handler()();  // Invoke the handler.

  return EVHTP_RES_OK;
}

evhtp_res on_read_response(evhtp_request_t *p_req, evbuf_t *p_buf,
                           void *p_arg) {
  int resp_status;
  unsigned addb_type;

  assert(p_arg != nullptr);
  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);

  if (S3AuthClientOpType::authentication == p_auth_ctx->op_type &&
      s3_fi_is_enabled("fake_authentication_fail")) {
    resp_status = S3HttpFailed401;
  } else {
    assert(p_req != nullptr);
    resp_status = evhtp_request_status(p_req);
  }
  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(),
         "Response status from Auth server is %d\n", resp_status);

  switch (p_auth_ctx->op_type) {
    case S3AuthClientOpType::authorization:
      addb_type = ACTS_AUTH_OP_ON_AUTHZ_RESP;
      break;
    case S3AuthClientOpType::combo_auth:
      addb_type = ACTS_AUTH_OP_ON_AUTHZ_RESP;
      break;
    case S3AuthClientOpType::aclvalidation:
      addb_type = ACTS_AUTH_OP_ON_ACL_RESP;
      break;
    case S3AuthClientOpType::policyvalidation:
      addb_type = ACTS_AUTH_OP_ON_POLICY_RESP;
      break;
    default:
      addb_type = ACTS_AUTH_OP_ON_AUTH_RESP;
  }
  auto request_inst = p_auth_ctx->get_request();

  ADDB(S3_ADDB_AUTH_ID, request_inst->addb_request_id, addb_type);

  // Note: Do not remove this, else you will have s3 crashes as the
  // callbacks are invoked after request/connection is freed.
  p_auth_ctx->unset_hooks();

  if (!request_inst->client_connected()) {
    // S3 client has already disconnected, ignore
    s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(),
           "S3 Client has already disconnected.\n");

    // Calling failed handler to do proper cleanup to avoid leaks
    // i.e cleanup of S3Request and respective action chain.
    p_auth_ctx->on_failed_handler()();  // Invoke the handler.
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);

    return EVHTP_RES_OK;
  }
  assert(p_buf != nullptr);
  const auto buffer_len = evbuffer_get_length(p_buf);

  char *auth_response_body = (char *)::malloc(buffer_len + 1);
  if (auth_response_body == NULL) {
    s3_log(S3_LOG_FATAL, p_auth_ctx->get_request_id(),
           "malloc() returned NULL\n");
  }
  const auto nread = evbuffer_copyout(p_buf, auth_response_body, buffer_len);
  auth_response_body[nread > 0 ? nread : 0] = '\0';

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(),
         "Response data received from Auth service = %s\n", auth_response_body);

  p_auth_ctx->handle_response(auth_response_body, resp_status);
  ::free(auth_response_body);

  if (S3HttpSuccess200 == resp_status) {
    p_auth_ctx->on_success_handler()();
  } else {
    p_auth_ctx->on_failed_handler()();  // Invoke the handler.
  }
  return EVHTP_RES_OK;
}

static void timeout_cb_auth_retry(evutil_socket_t fd, short event, void *arg) {
  struct event_auth_timeout_arg *timeout_arg =
      (struct event_auth_timeout_arg *)arg;
  S3AuthClient *auth_client = (S3AuthClient *)(timeout_arg->auth_client);

  auto request = auth_client->get_request();
  auto request_id = request->get_request_id();
  auto stripped_request_id = request->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id, "Retring to connect to Auth server\n");

  ADDB(S3_ADDB_AUTH_ID, auth_client->get_request()->addb_request_id,
       ACTS_AUTH_OP_ON_TIMEOUT);

  unsigned addb_type;

  switch (auth_client->get_op_type()) {
    case S3AuthClientOpType::authorization:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTHZ_R;
      break;
    case S3AuthClientOpType::combo_auth:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTHZ_R;
      break;
    default:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTH_R;
  }
  ADDB_AUTH(addb_type);
  auth_client->trigger_request();

  event_free((struct event *)timeout_arg->event);
  free(timeout_arg);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

static void on_event_hook(evhtp_connection_t *conn, short events, void *p_arg) {

  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);
  auto addb_request_id = p_auth_ctx->get_request()->addb_request_id;

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(),
         "event (%x) EOF:%d ER:%d T:%d C:%d\n", (int)events,
         (int)!!(events & BEV_EVENT_EOF), (int)!!(events & BEV_EVENT_ERROR),
         (int)!!(events & BEV_EVENT_TIMEOUT),
         (int)!!(events & BEV_EVENT_CONNECTED));

  if (events & BEV_EVENT_CONNECTED) {
    ADDB(S3_ADDB_AUTH_ID, addb_request_id, ACTS_AUTH_OP_ON_EVENT_CONNECTED);
  }

  if (events & BEV_EVENT_ERROR) {
    ADDB(S3_ADDB_AUTH_ID, addb_request_id, ACTS_AUTH_OP_ON_EVENT_ERROR);
  }

  if (events & BEV_EVENT_TIMEOUT) {
    ADDB(S3_ADDB_AUTH_ID, addb_request_id, ACTS_AUTH_OP_ON_EVENT_TIMEOUT);
  }

  if (events & BEV_EVENT_EOF) {
    ADDB(S3_ADDB_AUTH_ID, addb_request_id, ACTS_AUTH_OP_ON_EVENT_EOF);
  }
}

static void on_write_hook(evhtp_connection_t *conn, void *p_arg) {
  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(), "on socket write\n");

  ADDB(S3_ADDB_AUTH_ID, p_auth_ctx->get_request()->addb_request_id,
       ACTS_AUTH_OP_ON_WRITE);
}

static evhtp_res on_response_headers_start(evhtp_request_t *r, void *p_arg) {

  assert(p_arg != NULL);
  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(),
         "Response from Auth server: headers start");

  ADDB(S3_ADDB_AUTH_ID, p_auth_ctx->get_request()->addb_request_id,
       ACTS_AUTH_OP_ON_HEADERS_READ);

  return EVHTP_RES_OK;
}

static evhtp_res on_response_header(evhtp_request_t *,
                                    evhtp_header_t *p_evhtp_hdr,
                                    void *p_arg) noexcept {
  assert(p_evhtp_hdr != NULL);
  assert(p_evhtp_hdr->key != NULL);
  assert(p_evhtp_hdr->val != NULL);
  assert(p_arg != NULL);

  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_stripped_request_id(),
         "Response's header from Auth Server - %s: %s", p_evhtp_hdr->key,
         p_evhtp_hdr->val);

  if (!strcasecmp(p_evhtp_hdr->key, "Content-Length")) {
    p_auth_ctx->save_content_length(atol(p_evhtp_hdr->val));
  }
  return EVHTP_RES_OK;
}

static evhtp_res on_response_headers_end(evhtp_request_t *p_evhtp_req,
                                         evhtp_headers_t *,
                                         void *p_arg) noexcept {
  assert(p_arg != NULL);
  auto *p_auth_ctx = static_cast<S3AuthClientOpContext *>(p_arg);

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_stripped_request_id(),
         "Response from Auth server: headers end");

  const int resp_status = evhtp_request_status(p_evhtp_req);

  s3_log(S3_LOG_DEBUG, p_auth_ctx->get_request_id(),
         "HTTP response status from Auth server - %d", resp_status);

  const auto content_lentgh = p_auth_ctx->get_content_length();

  if (content_lentgh == -1) {
    s3_log(S3_LOG_WARN, p_auth_ctx->get_request_id(),
           "S3AuthClient doesn't return \"Content-Length\" header");
  } else if (content_lentgh == 0) {

    switch (p_auth_ctx->op_type) {
      case S3AuthClientOpType::aclvalidation:
      case S3AuthClientOpType::policyvalidation:

        if (S3HttpSuccess200 == resp_status) {
          // s3_auth_op_success(p_auth_ctx, 0);  // Invoke the handler.
          p_auth_ctx->on_success_handler()();
        } else {
          p_auth_ctx->on_failed_handler()();  // Invoke the handler.
        }
        break;
      default:
        s3_log(S3_LOG_ERROR, p_auth_ctx->get_request_id(),
               "Auth server returns response without body");
        p_auth_ctx->set_auth_response_error(
            "InternalError", "Incorrect response from Auth server",
            p_auth_ctx->get_request_id());

        p_auth_ctx->on_failed_handler()();  // Invoke the handler.
    }
  }
  return EVHTP_RES_OK;
}

// S3AuthClientOpContext

S3AuthClientOpContext::S3AuthClientOpContext(
    std::shared_ptr<RequestObject> req, std::function<void()> success_callback,
    std::function<void()> failed_callback, S3AuthClientOpType op_type)
    : S3AsyncOpContextBase(std::move(req), std::move(success_callback),
                           std::move(failed_callback)),
      op_type(op_type) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  ADDB_AUTH(ACTS_AUTH_OP_CTX_CONSTRUCT);
}

S3AuthClientOpContext::~S3AuthClientOpContext() {
  s3_log(S3_LOG_DEBUG, request_id, "%s\n", __func__);
  ADDB_AUTH(ACTS_AUTH_OP_CTX_DESTRUCT);
  clear_op_context();
}

const char *S3AuthClientOpContext::get_auth_status_message(int http_status)
    const noexcept {
  switch (op_type) {
    case S3AuthClientOpType::authentication:

      switch (http_status) {
        case S3HttpSuccess200:
          return "Authentication successful";
        case S3HttpFailed400:
          return "Authentication failed:Bad Request";
        case S3HttpFailed401:
          return "Authentication failed";
        case S3HttpFailed403:
          return "Authentication failed:Access denied";
      }
      break;
    case S3AuthClientOpType::authorization:

      switch (http_status) {
        case S3HttpSuccess200:
          return "Authorization successful";
        case S3HttpFailed400:
          return "Authorization failed: BadRequest";
        case S3HttpFailed401:
          return "Authorization failed";
        case S3HttpFailed403:
          return "Authorization failed: AccessDenied";
        case S3HttpFailed405:
          return "Authorization failed: Method Not Allowed";
      }
      break;
    case S3AuthClientOpType::combo_auth:

      switch (http_status) {
        case S3HttpSuccess200:
          return "Combo Auth successful";
        case S3HttpFailed400:
          return "Combo Auth failed: BadRequest";
        case S3HttpFailed401:
          return "Combo Auth failed";
        case S3HttpFailed403:
          return "Combo Auth failed: AccessDenied";
        case S3HttpFailed405:
          return "Combo Auth failed: Method Not Allowed";
      }
      break;
    case S3AuthClientOpType::aclvalidation:

      switch (http_status) {
        case S3HttpSuccess200:
          return "Aclvalidation successful";
        case S3HttpFailed401:
          return "Aclvalidation failed";
        case S3HttpFailed405:
          return "AclValidation failed: Method Not Allowed";
      }
      break;
    case S3AuthClientOpType::policyvalidation:

      switch (http_status) {
        case S3HttpSuccess200:
          return "Policy validation successful";
        case S3HttpFailed401:
          return "Policy validation failed";
        case S3HttpFailed405:
          return "Policy validation failed: Method Not Allowed";
      }
      break;
    default:
      s3_log(S3_LOG_WARN, request_id, "Unknown S3AuthClientOpType");
  }
  return "Something is wrong with Auth server";
}

void S3AuthClientOpContext::handle_response(const char *sz_resp,
                                            int http_status) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  assert(sz_resp != nullptr);
  s_resp.assign(sz_resp);

  f_success = (S3HttpSuccess200 == http_status);
  const char *sz_auth_msg = get_auth_status_message(http_status);

  if (f_success) {
    s3_log(S3_LOG_DEBUG, request_id, "%s\n", sz_auth_msg);
  } else {
    s3_log(S3_LOG_ERROR, request_id, "%s\n", sz_auth_msg);
  }
  set_op_status_for(
      0, f_success ? S3AsyncOpStatus::success : S3AsyncOpStatus::failed,
      sz_auth_msg);

  if (!f_success) {
    error_obj.reset(new S3AuthResponseError(s_resp));
    s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
    return;
  }
  switch (op_type) {
    case S3AuthClientOpType::aclvalidation:
    case S3AuthClientOpType::policyvalidation:
      s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
      return;
    default:
      ;  // -Wall
  }
  success_obj.reset(new S3AuthResponseSuccess(s_resp));

  if (!success_obj->isOK()) {
    s3_log(S3_LOG_ERROR, request_id, "Incorrect response from Auth server");
    f_success = false;
    return;
  }
  if (S3AuthClientOpType::combo_auth == op_type ||
      S3AuthClientOpType::authentication == op_type) {

    request->set_user_name(success_obj->get_user_name());
    request->set_canonical_id(success_obj->get_canonical_id());
    request->set_user_id(success_obj->get_user_id());
    request->set_account_name(success_obj->get_account_name());
    request->set_account_id(success_obj->get_account_id());
    request->set_email(success_obj->get_email());
  }
  if (S3AuthClientOpType::authentication == op_type) {
    s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
    return;
  }
  switch (op_type) {
    case S3AuthClientOpType::authorization:
    case S3AuthClientOpType::combo_auth:
      break;
    default:
      s3_log(S3_LOG_ERROR, request_id, "%s LOGIC ERROR: auth op type is missed",
             __func__);
  }
  std::shared_ptr<S3RequestObject> s3_request =
      std::dynamic_pointer_cast<S3RequestObject>(request);

  if (!s3_request) {
    s3_log(S3_LOG_ERROR, request_id, "Illegal type of Request instance");
    f_success = false;  // since auth response details are bad
    return;
  }
  s3_request->set_default_acl(success_obj->get_acl());

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClientOpContext::set_auth_response_error(std::string error_code,
                                                    std::string error_message,
                                                    std::string request_id) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  error_obj.reset(new S3AuthResponseError(
      std::move(error_code), std::move(error_message), request_id));
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
}

bool S3AuthClientOpContext::init_auth_op_ctx() {
  struct event_base *p_event_base = S3Option::get_instance()->get_eventbase();

  if (!p_event_base) {
    s3_log(S3_LOG_FATAL, request_id, "%s EventBase handler is NULL", __func__);
  }

  ADDB_AUTH(ACTS_AUTH_OP_CTX_NEW_CONN);
  clear_op_context();

  auth_op_context = create_basic_auth_op_ctx(p_event_base);

  if (auth_op_context == NULL) {
    return false;
  }
  evhtp_set_hook(&auth_op_context->auth_request->hooks, evhtp_hook_on_read,
                 (evhtp_hook)on_read_response, this);
  evhtp_set_hook(&auth_op_context->conn->hooks, evhtp_hook_on_event,
                 (evhtp_hook)on_event_hook, (void *)this);
  evhtp_set_hook(&auth_op_context->conn->hooks, evhtp_hook_on_write,
                 (evhtp_hook)on_write_hook, (void *)this);
  evhtp_set_hook(&auth_op_context->auth_request->hooks,
                 evhtp_hook_on_headers_start,
                 (evhtp_hook)on_response_headers_start, this);
  evhtp_set_hook(&auth_op_context->auth_request->hooks, evhtp_hook_on_header,
                 (evhtp_hook)on_response_header, this);
  evhtp_set_hook(&auth_op_context->auth_request->hooks, evhtp_hook_on_headers,
                 (evhtp_hook)on_response_headers_end, this);
  evhtp_set_hook(&auth_op_context->conn->hooks, evhtp_hook_on_conn_error,
                 (evhtp_hook)on_conn_err_callback, (void *)this);

  return true;
}

void S3AuthClientOpContext::unset_hooks() {
  if (auth_op_context) {
    if (auth_op_context->conn) {
      evhtp_unset_all_hooks(&auth_op_context->conn->hooks);
    }
    if (auth_op_context->auth_request) {
      evhtp_unset_all_hooks(&auth_op_context->auth_request->hooks);
    }
  }
}

void S3AuthClientOpContext::clear_op_context() {
  free_basic_auth_client_op_ctx(auth_op_context);
  auth_op_context = NULL;
}

// S3AuthClient

S3AuthClient::S3AuthClient(std::shared_ptr<RequestObject> req,
                           bool skip_authorization)
    : request(std::move(req)), skip_authorization(skip_authorization) {

  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  ADDB_AUTH(ACTS_AUTH_CLNT_CONSTRUCT);
}

S3AuthClient::~S3AuthClient() {
  s3_log(S3_LOG_DEBUG, request_id, "%s\n", __func__);
  ADDB_AUTH(ACTS_AUTH_CLNT_DESTRUCT);
}

void S3AuthClient::add_key_val_to_body(std::string key, std::string val) {
  data_key_val[std::move(key)] = std::move(val);
}

void S3AuthClient::add_non_empty_key_val_to_body(std::string key,
                                                 std::string val) {
  if (!val.empty()) {
    add_key_val_to_body(std::move(key), std::move(val));
  }
}

void S3AuthClient::add_non_empty_key_val_to_body(
    std::string key, const std::list<std::string> &val_list) {
  std::string val_str;
  for (const auto &val : val_list) {
    if (!val.empty()) {
      val_str += val + " ";
    }
  }
  val_str.pop_back();  // remove last space
  add_key_val_to_body(std::move(key), std::move(val_str));
}

void S3AuthClient::set_event_with_retry_interval() {
  S3Option *option_instance = S3Option::get_instance();

  auto interval = option_instance->get_retry_interval_in_millisec();
  interval = interval * retry_count;

  struct timeval s_tv;
  s_tv.tv_sec = interval / 1000;
  s_tv.tv_usec = (interval % 1000) * 1000;

  // Will be freed in callback
  struct event_auth_timeout_arg *arg = (struct event_auth_timeout_arg *)calloc(
      1, sizeof(struct event_auth_timeout_arg));
  arg->auth_client = this;
  struct event *ev = event_new(option_instance->get_eventbase(), -1, 0,
                               timeout_cb_auth_retry, (void *)arg);

  arg->event = ev;  // Will be freed in callback
  event_add(ev, &s_tv);
}

std::string S3AuthClient::get_signature_from_response() {
  return auth_context->is_success() ? auth_context->get_signature_sha256() : "";
}

std::string S3AuthClient::get_error_message() const {
  return auth_context->is_success() ? "" : auth_context->get_error_message();
}

// Returns AccessDenied | InvalidAccessKeyId | SignatureDoesNotMatch
// auth InactiveAccessKey maps to InvalidAccessKeyId in S3
std::string S3AuthClient::get_error_code() const {

  if (!auth_context->is_success()) {
    const auto &code = auth_context->get_error_code();

    if (code == "InactiveAccessKey") {
      return "InvalidAccessKeyId";
    } else if (code == "ExpiredCredential") {
      return "ExpiredToken";
    } else if (code == "InvalidClientTokenId") {
      return "InvalidToken";
    } else if (code == "TokenRefreshRequired") {
      return "AccessDenied";
    } else if (code == "AccessDenied") {
      return "AccessDenied";
    } else if (code == "MethodNotAllowed") {
      return "MethodNotAllowed";
    } else if (!code.empty()) {
      return code;  // InvalidAccessKeyId | SignatureDoesNotMatch
    }
  }
  return "ServiceUnavailable";  // auth server may be down
}

bool S3AuthClient::setup_auth_request_body() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  // Setup the headers to forward to auth service
  s3_log(S3_LOG_DEBUG, request_id, "Headers from S3 client:\n");

  data_key_val.clear();

  if (op_type != S3AuthClientOpType::policyvalidation) {

    for (const auto &hdr_val : request->get_in_headers_copy()) {
      const auto &s_hdr = hdr_val.first;
      const auto &s_val = hdr_val.second;

      s3_log(S3_LOG_DEBUG, request_id, "%s: %s\n", s_hdr.c_str(),
             s_val.c_str());
      add_key_val_to_body(s_hdr, s_val);
    }
  }
  std::string auth_request_body;
  std::string method;

  switch (request->http_verb()) {
    case S3HttpVerb::GET:
      method = "GET";
      break;
    case S3HttpVerb::HEAD:
      method = "HEAD";
      break;
    case S3HttpVerb::PUT:
      method = "PUT";
      break;
    case S3HttpVerb::DELETE:
      method = "DELETE";
      break;
    case S3HttpVerb::POST:
      method = "POST";
      break;
    default:
      ;  // -Wall
  }
  if (set_get_method) {
    method = "GET";
  }
  add_key_val_to_body("Method", method);
  add_key_val_to_body("RequestorAccountId", request->get_account_id());
  add_key_val_to_body("RequestorAccountName", request->get_account_name());
  add_key_val_to_body("RequestorUserId", request->get_user_id());
  add_key_val_to_body("RequestorUserName", request->get_user_name());
  add_key_val_to_body("RequestorEmail", request->get_email());
  add_key_val_to_body("RequestorCanonicalId", request->get_canonical_id());

  std::shared_ptr<S3RequestObject> s3_request =
      std::dynamic_pointer_cast<S3RequestObject>(request);

  if (entity_path.empty()) {

    const char *sz_full_path = request->c_get_full_encoded_path();

    if (sz_full_path) {
      entity_path.assign(sz_full_path);

      // in encoded uri path space is encoding as '+'
      // but cannonical request should have '%20' for verification.
      // decode plus into space, this special handling
      // is not required for other charcters.
      S3CommonUtilities::find_and_replaceall(entity_path, "+", "%20");

      std::string authorization_header =
          request->get_header_value("Authorization");

      if (authorization_header.rfind("AWS4-HMAC-SHA256", 0) == 0) {
        S3CommonUtilities::find_and_replaceall(entity_path, "!", "%21");
      }
    }
  }
  if (entity_path == "/" && S3AuthClientOpType::policyvalidation == op_type) {
    entity_path.append(s3_request->get_bucket_name());
  }
  add_key_val_to_body("ClientAbsoluteUri", entity_path);

  // get the query paramters in a map
  // eg: query_map = { {prefix, abc&def}, {delimiter, /}};
  const std::map<std::string, std::string, compare> query_map =
      request->get_query_parameters();
  std::string query;
  // iterate through the each query parameter
  // and url-encode the values (abc%26def)
  // then form the query (prefix=abc%26def&delimiter=%2F)
  bool f_evhttp_encode_uri_succ = true;

  for (auto it : query_map) {
    if (it.second == "") {
      query += query.empty() ? it.first : "&" + it.first;
    } else {

      char *encoded_value = evhttp_encode_uri(it.second.c_str());
      if (!encoded_value) {
        s3_log(S3_LOG_ERROR, request_id, "Failed to URI encode value = %s",
               it.second.c_str());
        f_evhttp_encode_uri_succ = false;
        break;
      }

      std::string encoded_value_str = encoded_value;
      query += query.empty() ? it.first + "=" + encoded_value_str
                             : "&" + it.first + "=" + encoded_value_str;
      free(encoded_value);
    }
  }

  if (!f_evhttp_encode_uri_succ) {
    s3_log(S3_LOG_ERROR, request_id,
           "evhttp_encode_uri() returned NULL for client query params");
    auth_request_body.clear();
    return false;
  }

  add_key_val_to_body("ClientQueryParams", query);

  // May need to take it from config
  add_key_val_to_body("Version", "2010-05-08");
  add_key_val_to_body("Request_id", request_id);

  if (op_type == S3AuthClientOpType::authorization ||
      op_type == S3AuthClientOpType::combo_auth) {

    if (s3_request) {

      // Set flag to request default bucket acl from authserver.
      if ((s3_request->http_verb() == S3HttpVerb::PUT &&
           s3_request->get_operation_code() == S3OperationCode::none &&
           s3_request->get_api_type() == S3ApiType::bucket)) {

        add_key_val_to_body("Request-ACL", "true");
      }

      // Set flag to request default object acl for api put-object, put object
      // in complete multipart upload and put object in chunked mode
      // from authserver.
      else if (((s3_request->http_verb() == S3HttpVerb::PUT &&
                 s3_request->get_operation_code() == S3OperationCode::none) ||
                (s3_request->http_verb() == S3HttpVerb::POST &&
                 s3_request->get_operation_code() ==
                     S3OperationCode::multipart)) &&
               s3_request->get_api_type() == S3ApiType::object) {
        add_key_val_to_body("Request-ACL", "true");
      } else if ((s3_request->http_verb() == S3HttpVerb::PUT &&
                  s3_request->get_operation_code() == S3OperationCode::acl) &&
                 ((s3_request->get_api_type() == S3ApiType::object) ||
                  (s3_request->get_api_type() == S3ApiType::bucket))) {
        add_key_val_to_body("Request-ACL", "true");
      }
      // PUT Object ACL case
      else if (s3_request->http_verb() == S3HttpVerb::PUT &&
               s3_request->get_operation_code() == S3OperationCode::acl) {
        add_key_val_to_body("Request-ACL", "true");
      }
      if (set_get_method) {
        add_key_val_to_body("Request-ACL", "false");
      }
      add_non_empty_key_val_to_body("S3Action", s3_request->get_action_str());
      add_non_empty_key_val_to_body("S3ActionList",
                                    s3_request->get_action_list());
    }
    add_non_empty_key_val_to_body("Policy", policy_str);
    add_non_empty_key_val_to_body("Auth-ACL", acl_str);
    add_non_empty_key_val_to_body("Bucket-ACL", bucket_acl);

    if (op_type == S3AuthClientOpType::combo_auth) {
      auth_request_body = "Action=AuthenticateAndAuthorize";
    } else {
      auth_request_body = "Action=AuthorizeUser";
    }
  } else if (op_type == S3AuthClientOpType::authentication) {  // Auth request

    if (is_chunked_auth && !(prev_chunk_signature_from_auth.empty() ||
                             current_chunk_signature_from_auth.empty())) {
      add_key_val_to_body("previous-signature-sha256",
                          prev_chunk_signature_from_auth);
      add_key_val_to_body("current-signature-sha256",
                          current_chunk_signature_from_auth);
      add_key_val_to_body("x-amz-content-sha256", hash_sha256_current_chunk);
    }

    auth_request_body = "Action=AuthenticateUser";
  } else if (op_type == S3AuthClientOpType::aclvalidation) {

    add_key_val_to_body("ACL", user_acl);
    add_key_val_to_body("Auth-ACL", acl_str);
    auth_request_body = "Action=ValidateACL";
  } else if (op_type == S3AuthClientOpType::policyvalidation) {
    add_key_val_to_body("Policy", policy_str);
    auth_request_body = "Action=ValidatePolicy";
  }

  for (auto &it : data_key_val) {

    char *encoded_key = evhttp_encode_uri(it.first.c_str());
    if (!encoded_key) {
      f_evhttp_encode_uri_succ = false;
      break;
    }
    auth_request_body += '&';
    auth_request_body += encoded_key;
    free(encoded_key);

    char *encoded_value = evhttp_encode_uri(it.second.c_str());
    if (!encoded_value) {
      f_evhttp_encode_uri_succ = false;
      break;
    }
    auth_request_body += '=';
    auth_request_body += encoded_value;
    free(encoded_value);
  }
  if (!f_evhttp_encode_uri_succ) {
    s3_log(S3_LOG_ERROR, request_id, "evhttp_encode_uri() returned NULL");
    auth_request_body.clear();
  }
  // evbuffer_add_printf(op_ctx->authrequest->buffer_out,
  // auth_request_body.c_str());
  req_body_buffer = evbuffer_new();
  evbuffer_add(req_body_buffer, auth_request_body.c_str(),
               auth_request_body.length());

  s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);

  return !auth_request_body.empty();
}

void S3AuthClient::set_acl_and_policy(const std::string &acl,
                                      const std::string &policy) {
  policy_str = policy;
  acl_str = acl;
}

void S3AuthClient::set_bucket_acl(const std::string &acl) { bucket_acl = acl; }

void S3AuthClient::set_validate_acl(const std::string &validateacl) {
  user_acl = validateacl;
}

void S3AuthClient::setup_auth_request_headers() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  char sz_size[32];
  const size_t out_len = evbuffer_get_length(req_body_buffer);

  if (snprintf(sz_size, sizeof(sz_size), "%zu", out_len) < 0) {
    s3_log(S3_LOG_FATAL, request_id, "%s Too small buffer", __func__);
  }
  s3_log(S3_LOG_DEBUG, request_id, "Header - Length = %zu\n", out_len);

  std::string host = request->get_host_header();
  evhtp_request_t *p_evhtp_req = auth_context->get_auth_op_ctx()->auth_request;

  if (!host.empty()) {
    evhtp_headers_add_header(p_evhtp_req->headers_out,
                             evhtp_header_new("Host", host.c_str(), 1, 1));
  }
  evhtp_headers_add_header(p_evhtp_req->headers_out,
                           evhtp_header_new("Content-Length", sz_size, 1, 1));
  evhtp_headers_add_header(
      p_evhtp_req->headers_out,
      evhtp_header_new("Content-Type", "application/x-www-form-urlencoded", 0,
                       0));

  evhtp_headers_add_header(p_evhtp_req->headers_out,
                           evhtp_header_new("User-Agent", "s3server", 1, 1));
  /* Using a new connection per request, all types of requests like
  *   authenticarion, authorization, validateacl, validatepolicy open
  *   a connection and it will be closed by Authserver.
  *   TODO : Evaluate performance and change this approach to reuse
  *   connections
  */

  evhtp_headers_add_header(p_evhtp_req->headers_out,
                           evhtp_header_new("Connection", "close", 1, 1));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClient::set_is_authheader_present(
    bool is_authorizationheader_present) {
  is_authheader_present = is_authorizationheader_present;
}

void S3AuthClient::execute_authconnect_request(
    struct s3_auth_op_context *auth_ctx) {

  ADDB_AUTH(ACTS_AUTH_CLNT_EXEC_AUTH_CONN);

  if ((op_type == S3AuthClientOpType::authorization &&
       s3_fi_is_enabled("fake_authorization_fail")) ||
      (op_type == S3AuthClientOpType::authentication &&
       s3_fi_is_enabled("fake_authentication_fail"))) {
    s3_auth_fake_evhtp_request(op_type, auth_context.get());
  } else {
    evhtp_make_request(auth_ctx->conn, auth_ctx->auth_request, htp_method_POST,
                       "/");
    evhtp_send_reply_body(auth_ctx->auth_request, req_body_buffer);
  }

  evbuffer_free(req_body_buffer);
  req_body_buffer = NULL;
}

void S3AuthClient::trigger_request() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  AtExit at_exit_on_error([this]() {
    if (auth_context) {
      auth_context->set_auth_response_error(
          "InternalError", "Error while preparing request to Auth server",
          request_id);
    }
    state = S3AuthClientOpState::failed;
    handler_on_failed();
  });
  auth_context->init_auth_op_ctx();

  // Setup the body to be sent to auth service
  if (!setup_auth_request_body()) {
    state = S3AuthClientOpState::failed;

    if (req_body_buffer) {
      evbuffer_free(req_body_buffer);
      req_body_buffer = NULL;
    }
    return;
  }
  setup_auth_request_headers();

  if (S3Option::get_instance()->get_log_level() == "DEBUG") {

    const size_t buffer_len = evbuffer_get_length(req_body_buffer);
    char *sz_request = (char *)::malloc(buffer_len + 1);

    if (!sz_request) {
      s3_log(S3_LOG_FATAL, request_id, "malloc() returned NULL\n");
    }
    const auto nread =
        evbuffer_copyout(req_body_buffer, sz_request, buffer_len);
    sz_request[nread > 0 ? nread : 0] = '\0';

    s3_log(S3_LOG_DEBUG, request_id, "Data being send to Auth server: = %s\n",
           sz_request);
    ::free(sz_request);
  }
  execute_authconnect_request(auth_context->get_auth_op_ctx());

  at_exit_on_error.cancel();
  state = S3AuthClientOpState::started;

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Note: S3AuthClientOpContext should be created before calling this.
// This is called by check_authentication and start_chunk_auth
void S3AuthClient::trigger_request(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  assert(on_success);
  assert(on_failed);

  if (op_type == S3AuthClientOpType::authentication ||
      op_type == S3AuthClientOpType::combo_auth) {

    if (s3_fi_is_enabled("fake_authentication_fail")) {
      auth_context->set_auth_response_error(
          "InvalidAccessKeyId", "Test of authentication failure", request_id);
      on_failed();

      s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
      return;
    }
  }
  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  retry_count = 0;

  trigger_request();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClient::validate_acl(std::function<void(void)> on_success,
                                std::function<void(void)> on_failed) {

  ADDB_AUTH(ACTS_AUTH_CLNT_VALIDATE_ACL);

  data_key_val.clear();

  auth_context.reset(new S3AuthClientOpContext(
      request, std::bind(&S3AuthClient::on_common_success, this),
      std::bind(&S3AuthClient::on_common_failed, this),
      op_type = S3AuthClientOpType::aclvalidation));

  trigger_request(std::move(on_success), std::move(on_failed));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClient::validate_policy(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {

  ADDB_AUTH(ACTS_AUTH_CLNT_VALIDATE_POLICY);

  auth_context.reset(new S3AuthClientOpContext(
      request, std::bind(&S3AuthClient::on_common_success, this),
      std::bind(&S3AuthClient::on_common_failed, this),
      op_type = S3AuthClientOpType::policyvalidation));

  trigger_request(std::move(on_success), std::move(on_failed));

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3AuthClient::on_common_success() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  state = S3AuthClientOpState::succeded;

  unsigned addb_type;

  switch (op_type) {
    case S3AuthClientOpType::authorization:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTHZ_SUCC;
      break;
    case S3AuthClientOpType::combo_auth:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTHZ_SUCC;
      break;
    case S3AuthClientOpType::aclvalidation:
      addb_type = ACTS_AUTH_CLNT_VALIDATE_ACL_SUCC;
      break;
    case S3AuthClientOpType::policyvalidation:
      addb_type = ACTS_AUTH_CLNT_VALIDATE_POLICY_SUCC;
      break;
    default:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTH_SUCC;
  }
  ADDB_AUTH(addb_type);

  if (is_chunked_auth) {
    assert(S3AuthClientOpType::authentication == op_type);
    prev_chunk_signature_from_auth = get_signature_from_response();
  }
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3AuthClient::on_common_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  state = S3AuthClientOpState::failed;

  unsigned addb_type;

  switch (op_type) {
    case S3AuthClientOpType::authorization:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTHZ_FAILED;
      break;
    case S3AuthClientOpType::combo_auth:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTHZ_FAILED;
      break;
    case S3AuthClientOpType::aclvalidation:
      addb_type = ACTS_AUTH_CLNT_VALIDATE_ACL_FAILED;
      break;
    case S3AuthClientOpType::policyvalidation:
      addb_type = ACTS_AUTH_CLNT_VALIDATE_POLICY_FAILED;
      break;
    default:
      addb_type = ACTS_AUTH_CLNT_CHK_AUTH_FAILED;
  }
  ADDB_AUTH(addb_type);

  S3AsyncOpStatus op_state = auth_context.get()->get_op_status_for(0);

  if (op_state == S3AsyncOpStatus::connection_failed) {
    s3_log(S3_LOG_WARN, request_id, "Unable to connect to Auth server");
    state = S3AuthClientOpState::connection_error;

    if (retry_count < S3Option::get_instance()->get_max_retry_count()) {

      ++retry_count;
      set_event_with_retry_interval();
      return;

    } else {
      s3_log(S3_LOG_ERROR, request_id,
             "Cannot connect to Auth server (Retry count = %d).\n",
             retry_count);
      // s3_iem(LOG_ERR, S3_IEM_AUTH_CONN_FAIL, S3_IEM_AUTH_CONN_FAIL_STR,
      //     S3_IEM_AUTH_CONN_FAIL_JSON);
    }
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Auth/validation failure\n");
    state = S3AuthClientOpState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClient::check_authorization(std::function<void(void)> on_success,
                                       std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  ADDB_AUTH(ACTS_AUTH_CLNT_CHK_AUTHZ);

  auth_context.reset(new S3AuthClientOpContext(
      request, std::bind(&S3AuthClient::on_common_success, this),
      std::bind(&S3AuthClient::on_common_failed, this),
      op_type = S3AuthClientOpType::authorization));

  trigger_request(std::move(on_success), std::move(on_failed));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClient::check_combo_auth(std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  ADDB_AUTH(ACTS_AUTH_CLNT_CHK_AUTHZ);

  auth_context.reset(new S3AuthClientOpContext(
      request, std::bind(&S3AuthClient::on_common_success, this),
      std::bind(&S3AuthClient::on_common_failed, this),
      op_type = S3AuthClientOpType::combo_auth));

  for (const auto &hdr_val : request->get_in_headers_copy()) {
    if (!strcasecmp(hdr_val.first.c_str(), "Authorization")) {
      std::string to_find("Signature");
      const auto pos = hdr_val.second.find(to_find);

      if (std::string::npos == pos) {
        s3_log(S3_LOG_WARN, request_id,
               "\"Authorization\" header doesn't contain \"Signature\" value");
      } else {
        prev_chunk_signature_from_auth =
            hdr_val.second.substr(pos + to_find.length() + 1, 64);
        // 64 is the length of SHA256's string in hexadecimal
        s3_log(S3_LOG_DEBUG, request_id, "Signature=%s",
               prev_chunk_signature_from_auth.c_str());
      }
      break;
    }
  }
  trigger_request(std::move(on_success), std::move(on_failed));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047001001</event_code>
 *    <application>S3 Server</application>
 *    <submodule>Auth</submodule>
 *    <description>Auth server connection failed</description>
 *    <audience>Service</audience>
 *    <details>
 *      Not able to connect to the Auth server.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files. Restart S3 Auth server and check the
 *      auth server log files for any startup issues. If problem persists,
 *      contact development team for further investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

void S3AuthClient::check_authentication(std::function<void(void)> on_success,
                                        std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  ADDB_AUTH(ACTS_AUTH_CLNT_CHK_AUTH);
  auth_context.reset(new S3AuthClientOpContext(
      request, std::bind(&S3AuthClient::on_common_success, this),
      std::bind(&S3AuthClient::on_common_failed, this),
      op_type = S3AuthClientOpType::authentication));

  is_chunked_auth = false;

  trigger_request(std::move(on_success), std::move(on_failed));

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

// This is same as above but will cycle through with the
// add_checksum_for_chunk()
// to validate each chunk we receive.
void S3AuthClient::init_chunk_auth_cycle(std::function<void(void)> on_success,
                                         std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  ADDB_AUTH(ACTS_AUTH_CLNT_CHUNK_AUTH_INIT);

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  auth_context.reset(new S3AuthClientOpContext(
      request, std::bind(&S3AuthClient::chunk_auth_successful, this),
      std::bind(&S3AuthClient::chunk_auth_failed, this),
      op_type = S3AuthClientOpType::authentication));

  retry_count = 0;
  is_chunked_auth = true;
  state = S3AuthClientOpState::init;

  s3_log(S3_LOG_DEBUG, request_id, "%s Exit", __func__);
  // trigger is done when sign and hash are available for any chunk
}

// Insert the signature sent in each chunk (chunk-signature=value)
// and sha256(chunk-data) to be used in auth of chunk
void S3AuthClient::add_checksum_for_chunk(std::string current_sign,
                                          std::string sha256_of_payload) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  ADDB_AUTH(ACTS_AUTH_CLNT_CHUNK_ADD_CHKSUMM);

  chunk_validation_data.push(
      std::make_pair(std::move(current_sign), std::move(sha256_of_payload)));

  if (state != S3AuthClientOpState::started) {

    current_chunk_signature_from_auth =
        std::move(chunk_validation_data.front().first);
    hash_sha256_current_chunk = std::move(chunk_validation_data.front().second);

    chunk_validation_data.pop();

    trigger_request();
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3AuthClient::add_last_checksum_for_chunk(std::string current_sign,
                                               std::string sha256_of_payload) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  last_chunk_added = true;
  add_checksum_for_chunk(std::move(current_sign), std::move(sha256_of_payload));

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3AuthClient::chunk_auth_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  state = S3AuthClientOpState::succeded;

  ADDB_AUTH(ACTS_AUTH_CLNT_CHUNK_AUTH_SUCC);
  // remember_auth_details_in_request();
  prev_chunk_signature_from_auth = get_signature_from_response();

  if (chunk_auth_aborted ||
      (last_chunk_added && chunk_validation_data.empty())) {
    // we are done with all validations
    this->handler_on_success();

  } else if (chunk_validation_data.empty()) {
    // we have to wait for more chunk signatures, which will be
    // added with add_checksum_for_chunk/add_last_checksum_for_chunk
  } else {
    // Validate next chunk signature
    current_chunk_signature_from_auth =
        std::move(chunk_validation_data.front().first);
    hash_sha256_current_chunk = std::move(chunk_validation_data.front().second);

    chunk_validation_data.pop();

    trigger_request();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AuthClient::chunk_auth_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  state = S3AuthClientOpState::failed;

  ADDB_AUTH(ACTS_AUTH_CLNT_CHUNK_AUTH_FAILED);
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

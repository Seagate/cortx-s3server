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

#include <cstdlib>
#include <cassert>
#include <exception>
#include <algorithm>

#include <event2/util.h>
#include <event2/dns.h>

#include "s3_log.h"
#include "s3_http_post_queue_impl.h"
#include "s3_common_utilities.h"

S3HttpPostQueue::~S3HttpPostQueue() = default;
S3HttpPostEngine::~S3HttpPostEngine() = default;
S3HttpPostQueueImpl::~S3HttpPostQueueImpl() = default;

S3HttpPostQueueImpl::S3HttpPostQueueImpl(S3HttpPostEngine *http_post_engine_)
    : http_post_engine(http_post_engine_) {

  assert(http_post_engine_ != nullptr);
  http_post_engine->set_callbacks(
      std::bind(&S3HttpPostQueueImpl::on_msg_sent, this),
      std::bind(&S3HttpPostQueueImpl::on_error, this));
}

bool S3HttpPostQueueImpl::post(std::string msg) {
  if (n_err >= MAX_ERR) {
    s3_log(S3_LOG_DEBUG, nullptr, "Too many errors. Posting is stopped");
    return false;
  }
  if (msg.empty()) {
    s3_log(S3_LOG_INFO, nullptr, "Empty messages are not allowed");
    return false;
  }
  if (msg_queue.size() >= MAX_MSG_IN_QUEUE) {
    s3_log(S3_LOG_DEBUG, nullptr, "Too many messages in the queue");
    return false;
  }
  msg_queue.push(std::move(msg));

  if (!msg_in_progress) {
    send_front();
  } else {
    s3_log(S3_LOG_DEBUG, nullptr, "Another message is being processed");
  }
  return true;
}

void S3HttpPostQueueImpl::on_msg_sent() {
  assert(msg_in_progress);
  assert(!msg_queue.empty());

  msg_in_progress = false;
  n_err = 0;
  msg_queue.pop();

  if (!msg_queue.empty()) {
    s3_log(S3_LOG_DEBUG, nullptr, "%zu message(s) in the queue",
           msg_queue.size());
    send_front();
  }
}

void S3HttpPostQueueImpl::on_error() {
  assert(msg_in_progress);
  assert(!msg_queue.empty());

  msg_in_progress = false;

  if (++n_err > MAX_ERR) {
    s3_log(S3_LOG_ERROR, nullptr,
           "The number of errors has exceeded the threshold");
    while (!msg_queue.empty()) {
      msg_queue.pop();
    }
  } else {
    s3_log(S3_LOG_DEBUG, nullptr,
           "Message hasn't been sent %u times. Repeat...", n_err);
    send_front();
  }
}

void S3HttpPostQueueImpl::send_front() {
  assert(!msg_in_progress);
  assert(!msg_queue.empty());

  bool ret = http_post_engine->post(msg_queue.front());
  assert(ret);
  (void)ret;

  msg_in_progress = true;
}

S3HttpPostEngineImpl::S3HttpPostEngineImpl(
    evbase_t *p_evbase_, std::string s_host_, uint16_t port_, std::string path_,
    std::map<std::string, std::string> headers_)
    : p_evbase(p_evbase_),
      s_host(std::move(s_host_)),
      port(port_),
      path(std::move(path_)),
      headers(std::move(headers_)) {}

S3HttpPostEngineImpl::~S3HttpPostEngineImpl() {

  if (p_conn) {
    if (p_conn->request) {
      evhtp_unset_all_hooks(&p_conn->request->hooks);
    }
    evhtp_unset_all_hooks(&p_conn->hooks);
  }
  if (p_evdns_base) {
    evdns_base_free(p_evdns_base, 0);
  }
  if (p_event) {
    event_del(p_event);
    event_free(p_event);
  }
}

bool S3HttpPostEngineImpl::set_callbacks(std::function<void(void)> on_success,
                                         std::function<void(void)> on_fail) {
  if (!on_success || !on_fail) {
    s3_log(S3_LOG_ERROR, nullptr, "Bad parameter(s)");
    return false;
  }
  this->on_success = std::move(on_success);
  this->on_fail = std::move(on_fail);

  return true;
}

evhtp_res S3HttpPostEngineImpl::on_conn_err_cb(evhtp_connection_t *p_conn,
                                               evhtp_error_flags errtype,
                                               void *p_arg) noexcept {
  assert(p_conn != nullptr);
  evhtp_unset_all_hooks(&p_conn->hooks);

  assert(p_arg != nullptr);
  auto *p_inst = static_cast<S3HttpPostEngineImpl *>(p_arg);

  try {
    if (errtype != (BEV_EVENT_READING | BEV_EVENT_EOF)) {
      std::string s_errtype =
          S3CommonUtilities::evhtp_error_flags_description(errtype);
      s3_log(
          S3_LOG_INFO, nullptr, "Socket error: %s, errno: %d, set errtype: %s",
          evutil_socket_error_to_string(evutil_socket_geterror(p_conn->sock)),
          evutil_socket_geterror(p_conn->sock), s_errtype.c_str());
    } else {
      s3_log(S3_LOG_DEBUG, nullptr,
             "Probably a client has closed the connection");
    }
    assert(p_conn == p_inst->p_conn);
    p_inst->p_conn = nullptr;

    p_inst->request_finished(p_conn->request);
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
  return EVHTP_RES_OK;
}

evhtp_res S3HttpPostEngineImpl::on_conn_fini_cb(evhtp_connection_t *p_conn,
                                                void *p_arg) noexcept {
  assert(p_conn != nullptr);
  assert(p_arg != nullptr);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "");
    auto *p_inst = static_cast<S3HttpPostEngineImpl *>(p_arg);

    if (p_inst->p_conn) {
      assert(p_inst->p_conn == p_conn);
      p_inst->p_conn = nullptr;
    }
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
  return EVHTP_RES_OK;
}

bool S3HttpPostEngineImpl::connect() {
  if (p_conn) {
    return true;
  }
  if (!p_evdns_base) {
    p_evdns_base = evdns_base_new(p_evbase, EVDNS_BASE_INITIALIZE_NAMESERVERS);

    if (!p_evdns_base) {
      s3_log(S3_LOG_ERROR, nullptr, "evdns_base_new() failed");
      return false;
    }
  }
  p_conn =
      evhtp_connection_new_dns(p_evbase, p_evdns_base, s_host.c_str(), port);

  if (!p_conn) {
    s3_log(S3_LOG_ERROR, nullptr, "evhtp_connection_new_dns() failed");
    return false;
  }
  evhtp_set_hook(&p_conn->hooks, evhtp_hook_on_conn_error,
                 (evhtp_hook)on_conn_err_cb, this);
  evhtp_set_hook(&p_conn->hooks, evhtp_hook_on_connection_fini,
                 (evhtp_hook)on_conn_fini_cb, this);
  return true;
}

evhtp_res S3HttpPostEngineImpl::on_headers_start_cb(
    evhtp_request_t *p_evhtp_req, void *p_arg) noexcept {
  assert(p_evhtp_req != nullptr);
  assert(p_arg != nullptr);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "HTTP status: %d",
           evhtp_request_status(p_evhtp_req));
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
  return EVHTP_RES_OK;
}

evhtp_res S3HttpPostEngineImpl::on_header_cb(evhtp_request_t *p_evhtp_req,
                                             evhtp_header_t *p_hdr,
                                             void *p_arg) noexcept {
  assert(p_evhtp_req != nullptr);
  assert(p_hdr != nullptr);
  assert(p_arg != nullptr);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "%s: %s", p_hdr->key, p_hdr->val);

    if (!strcasecmp(p_hdr->key, "Content-Length")) {
      auto *p_inst = static_cast<S3HttpPostEngineImpl *>(p_arg);
      p_inst->response_content_length = atol(p_hdr->val);
    }
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
  return EVHTP_RES_OK;
}

evhtp_res S3HttpPostEngineImpl::on_headers_cb(evhtp_request_t *p_evhtp_req,
                                              evhtp_headers_t *p_hdrs,
                                              void *p_arg) noexcept {
  assert(p_evhtp_req != nullptr);
  assert(p_arg != nullptr);

  try {
    auto *p_inst = static_cast<S3HttpPostEngineImpl *>(p_arg);

    if (!p_inst->response_content_length) {
      s3_log(S3_LOG_DEBUG, nullptr, "None data in the response");
      p_inst->request_finished(p_evhtp_req);
    } else {
      s3_log(S3_LOG_DEBUG, nullptr, "Waiting data...");
    }
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
  return EVHTP_RES_OK;
}

evhtp_res S3HttpPostEngineImpl::on_response_data_cb(
    evhtp_request_t *p_evhtp_req, evbuf_t *p_evbuf, void *p_arg) noexcept {
  assert(p_evhtp_req != nullptr);
  assert(p_evbuf != nullptr);
  assert(p_arg != nullptr);

  try {
    size_t n_bytes = evbuffer_get_length(p_evbuf);
    s3_log(S3_LOG_DEBUG, nullptr, "Bytes arrived: %zu", n_bytes);

    char pb[80];
    auto n_copied = evbuffer_copyout(p_evbuf, pb, sizeof(pb) - 1);

    if (n_copied > 0) {
      *std::find_if(pb, pb + n_copied, [](char &ch) { return ch < ' '; }) =
          '\0';
      s3_log(S3_LOG_DEBUG, nullptr, "%s", pb);
    }
    evbuffer_drain(p_evbuf, -1);

    auto *p_inst = static_cast<S3HttpPostEngineImpl *>(p_arg);
    p_inst->n_read += n_bytes;

    if (p_inst->n_read >= p_inst->response_content_length) {
      p_inst->request_finished(p_evhtp_req);
    }
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
  return EVHTP_RES_OK;
}

bool S3HttpPostEngineImpl::send_request(const std::string &msg) {
  assert(!request_in_progress);
  assert(!msg.empty());

  if (!connect()) {
    return false;
  }
  evbuf_t *p_ev_buf = evbuffer_new();

  if (!p_ev_buf) {
    s3_log(S3_LOG_ERROR, nullptr, "evbuffer_new() failed");
    return false;
  }
  evhtp_request_t *p_evhtp_req = evhtp_request_new(nullptr, nullptr);

  if (!p_evhtp_req) {
    s3_log(S3_LOG_ERROR, nullptr, "evhtp_request_new() failed");
    evbuffer_free(p_ev_buf);
    return false;
  }
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    evhtp_headers_add_header(
        p_evhtp_req->headers_out,
        evhtp_header_new(it->first.c_str(), it->second.c_str(), 0, 0));
  }
  if (!msg.empty()) {
    auto s_content_length = std::to_string(msg.length());

    evhtp_headers_add_header(
        p_evhtp_req->headers_out,
        evhtp_header_new("Content-Length", s_content_length.c_str(), 0, 1));
  }
  evhtp_set_hook(&p_evhtp_req->hooks, evhtp_hook_on_headers_start,
                 (evhtp_hook)on_headers_start_cb, this);
  evhtp_set_hook(&p_evhtp_req->hooks, evhtp_hook_on_header,
                 (evhtp_hook)on_header_cb, this);
  evhtp_set_hook(&p_evhtp_req->hooks, evhtp_hook_on_headers,
                 (evhtp_hook)on_headers_cb, this);
  evhtp_set_hook(&p_evhtp_req->hooks, evhtp_hook_on_read,
                 (evhtp_hook)on_response_data_cb, this);

  evbuffer_add(p_ev_buf, msg.c_str(), msg.length());

  evhtp_make_request(p_conn, p_evhtp_req, htp_method_POST, path.c_str());
  evhtp_send_reply_body(p_evhtp_req, p_ev_buf);

  evbuffer_free(p_ev_buf);

  response_content_length = 0;
  n_read = 0;
  request_in_progress = true;

  s3_log(S3_LOG_DEBUG, nullptr, "Request has been sent");

  return true;
}

void S3HttpPostEngineImpl::on_schedule_cb(evutil_socket_t, short,
                                          void *p_arg) noexcept {
  assert(p_arg != nullptr);
  auto *p_inst = static_cast<S3HttpPostEngineImpl *>(p_arg);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "");

    assert(p_inst->on_fail);

    if (p_inst->f_error) {
      p_inst->f_error = false;
      p_inst->on_fail();
      return;
    }
    assert(!p_inst->msg_to_send.empty());

    const bool ret = p_inst->send_request(p_inst->msg_to_send);
    p_inst->msg_to_send.clear();

    if (!ret) {
      p_inst->on_fail();
    }
  }
  catch (const std::exception &ex) {
    s3_log(S3_LOG_ERROR, nullptr, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, nullptr, "Non-standard C++ exception");
  }
}

static bool is_request_succeed(evhtp_request_t *p_evhtp_req) {
  if (!p_evhtp_req) {
    return false;
  }
  const auto status = evhtp_request_status(p_evhtp_req);
  return status >= EVHTP_RES_OK && status < EVHTP_RES_300;
}

void S3HttpPostEngineImpl::request_finished(evhtp_request_t *p_evhtp_req) {
  if (!request_in_progress) {
    s3_log(S3_LOG_DEBUG, nullptr, "Double invocation");
    return;
  }
  if (p_evhtp_req) {
    evhtp_unset_all_hooks(&p_evhtp_req->hooks);
  }
  if (is_request_succeed(p_evhtp_req)) {
    assert(on_success);
    on_success();
  } else {
    call_error_callback();
  }
  request_in_progress = false;
}

bool S3HttpPostEngineImpl::prepare_scheduling() {
  if (!p_event) {
    p_event = event_new(p_evbase, -1, 0, on_schedule_cb, this);

    if (!p_event) {
      s3_log(S3_LOG_ERROR, nullptr, "event_new() failed");
      assert(on_fail);
      on_fail();
    }
  }
  return p_event;
}

const struct timeval tv = {0, 100000};  // 1/10 sec

void S3HttpPostEngineImpl::call_error_callback() {
  if (!prepare_scheduling()) {
    return;
  }
  if (event_add(p_event, &tv)) {
    s3_log(S3_LOG_ERROR, nullptr, "event_add() failed");
    assert(on_fail);
    on_fail();
  } else {
    f_error = true;
  }
}

bool S3HttpPostEngineImpl::post(const std::string &msg) {

  if (!p_evbase) {
    // It seams UTs are running
    return true;
  }
  if (msg.empty()) {
    s3_log(S3_LOG_ERROR, nullptr, "Bad parameter");
    return false;
  }
  if (!on_success || !on_fail) {
    s3_log(S3_LOG_ERROR, nullptr, "Bad callback(s)");
    return false;
  }
  if (!request_in_progress) {
    if (send_request(msg)) {
      return true;
    }
  } else if (!msg_to_send.empty()) {
    s3_log(S3_LOG_ERROR, nullptr,
           "The previous message hasn't been handled yet");
    return false;
  } else {
    s3_log(S3_LOG_ERROR, nullptr, "Schedule posting");
    msg_to_send = msg;
  }
  if (prepare_scheduling()) {
    if (!msg_to_send.empty()) {
      event_active(p_event, 0, 1);
    } else {
      call_error_callback();
    }
  }
  return true;
}

template <>
S3HttpPostQueue *create_http_post_queue(
    evbase_t *p_evbase, std::string s_host, uint16_t port, std::string path,
    std::map<std::string, std::string> headers) {

  return new S3HttpPostQueueImpl(new S3HttpPostEngineImpl(
      p_evbase, std::move(s_host), port, std::move(path), std::move(headers)));
}

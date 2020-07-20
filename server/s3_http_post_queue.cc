/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author:  Evgeniy Brazhnikov   <evgeniy.brazhnikov@seagate.com>
 * Original creation date: 13-Jul-2020
 */

#include <cstdlib>
#include <cassert>
#include <exception>
#include <algorithm>

#include <event2/util.h>

#include "s3_log.h"
#include "s3_http_post_queue.h"
#include "s3_common_utilities.h"

S3HttpPostQueue::S3HttpPostQueue(evbase_t *p_evbase_, std::string s_ipa_,
                                 uint16_t port_, std::string path_)
    : p_evbase(p_evbase_),
      s_ipa(std::move(s_ipa_)),
      port(port_),
      path(std::move(path_)) {}

S3HttpPostQueue::~S3HttpPostQueue() {
  unset_all_hooks();

  if (p_event) {
    event_del(p_event);
    event_free(p_event);
  }
}

void S3HttpPostQueue::unset_all_hooks() {
  if (p_conn) {
    if (p_conn->request) {
      evhtp_unset_all_hooks(&p_conn->request->hooks);
    }
    evhtp_unset_all_hooks(&p_conn->hooks);
  }
}

bool S3HttpPostQueue::post(std::string msg) {
  if (!p_evbase) {
    // It seams UTs are running
    return true;
  }
  if (msg.empty()) {
    s3_log(S3_LOG_INFO, nullptr, "Empty messages are not allowed");
    return false;
  }
  if (msg_queue.size() > MAX_MSG_IN_QUEUE) {
    s3_log(S3_LOG_WARN, nullptr, "Message queue is full");
    return false;
  }
  msg_queue.push(std::move(msg));

  if (request_in_progress) {
    s3_log(S3_LOG_DEBUG, nullptr, "Another message is in progress");
    return true;
  }
  return post_msg_from_queue();
}

evhtp_res S3HttpPostQueue::on_conn_err_cb(evhtp_connection_t *p_conn,
                                          evhtp_error_flags errtype,
                                          void *p_arg) noexcept {
  assert(p_conn != nullptr);
  assert(p_arg != nullptr);

  try {
    auto *p_inst = static_cast<S3HttpPostQueue *>(p_arg);

    if (errtype != (BEV_EVENT_READING | BEV_EVENT_EOF)) {
      std::string s_errtype =
          S3CommonUtilities::evhtp_error_flags_description(errtype);
      s3_log(
          S3_LOG_INFO, nullptr, "Socket error: %s, errno: %d, set errtype: %s",
          evutil_socket_error_to_string(evutil_socket_geterror(p_conn->sock)),
          evutil_socket_geterror(p_conn->sock), s_errtype.c_str());

      ++p_inst->n_err;
    } else {
      s3_log(S3_LOG_DEBUG, nullptr,
             "Probably a client has closed the connection");
    }
    assert(p_conn == p_inst->p_conn);
    p_inst->p_conn = nullptr;

    p_inst->unset_all_hooks();
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

evhtp_res S3HttpPostQueue::on_conn_fini_cb(evhtp_connection_t *p_conn,
                                           void *p_arg) noexcept {
  assert(p_conn != nullptr);
  assert(p_arg != nullptr);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "callback");
    auto *p_inst = static_cast<S3HttpPostQueue *>(p_arg);

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

bool S3HttpPostQueue::connect() {
  if (p_conn) {
    return true;
  }
  if (n_err >= MAX_ERR) {
    s3_log(S3_LOG_DEBUG, nullptr,
           "The number of errors exceeded the threshold");
    return false;
  }
  p_conn = evhtp_connection_new(p_evbase, s_ipa.c_str(), port);

  if (!p_conn) {
    s3_log(S3_LOG_ERROR, nullptr, "evhtp_connection_new() failed");
    ++n_err;
    return false;
  }
  evhtp_set_hook(&p_conn->hooks, evhtp_hook_on_conn_error,
                 (evhtp_hook)on_conn_err_cb, this);
  evhtp_set_hook(&p_conn->hooks, evhtp_hook_on_connection_fini,
                 (evhtp_hook)on_conn_fini_cb, this);
  return true;
}

evhtp_res S3HttpPostQueue::on_headers_start_cb(evhtp_request_t *p_evhtp_req,
                                               void *p_arg) noexcept {
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

evhtp_res S3HttpPostQueue::on_header_cb(evhtp_request_t *p_evhtp_req,
                                        evhtp_header_t *p_hdr,
                                        void *p_arg) noexcept {
  assert(p_evhtp_req != nullptr);
  assert(p_hdr != nullptr);
  assert(p_arg != nullptr);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "%s: %s", p_hdr->key, p_hdr->val);

    if (!strcasecmp(p_hdr->key, "Content-Length")) {
      auto *p_inst = static_cast<S3HttpPostQueue *>(p_arg);
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

evhtp_res S3HttpPostQueue::on_headers_cb(evhtp_request_t *p_evhtp_req,
                                         evhtp_headers_t *p_hdrs,
                                         void *p_arg) noexcept {
  assert(p_evhtp_req != nullptr);
  assert(p_arg != nullptr);

  try {
    auto *p_inst = static_cast<S3HttpPostQueue *>(p_arg);

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

evhtp_res S3HttpPostQueue::on_response_data_cb(evhtp_request_t *p_evhtp_req,
                                               evbuf_t *p_evbuf,
                                               void *p_arg) noexcept {
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

    auto *p_inst = static_cast<S3HttpPostQueue *>(p_arg);
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

bool S3HttpPostQueue::post_msg_from_queue() {
  assert(!request_in_progress);
  assert(!msg_queue.empty());

  if (!connect()) {
    return false;
  }
  evbuf_t *p_ev_buf = evbuffer_new();

  if (!p_ev_buf) {
    s3_log(S3_LOG_ERROR, nullptr, "evbuffer_new() failed");
    ++n_err;
    return false;
  }
  evhtp_request_t *p_evhtp_req = evhtp_request_new(nullptr, nullptr);

  if (!p_evhtp_req) {
    s3_log(S3_LOG_ERROR, nullptr, "evhtp_request_new() failed");
    ++n_err;
    evbuffer_free(p_ev_buf);
    return false;
  }
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    evhtp_headers_add_header(
        p_evhtp_req->headers_out,
        evhtp_header_new(it->first.c_str(), it->second.c_str(), 0, 0));
  }
  const auto &msg = msg_queue.front();

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

  request_in_progress = true;
  response_content_length = 0;
  n_read = 0;

  return true;
}

void S3HttpPostQueue::add_header(std::string name, std::string value) {
  headers[std::move(name)] = std::move(value);
}

void S3HttpPostQueue::on_schedule_cb(evutil_socket_t, short events,
                                     void *p_arg) noexcept {
  assert(p_arg != nullptr);

  try {
    s3_log(S3_LOG_DEBUG, nullptr, "callback");
    auto *p_inst = static_cast<S3HttpPostQueue *>(p_arg);
    p_inst->post_msg_from_queue();
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

const struct timeval tv = {0, 100000};  // 0.1 sec

void S3HttpPostQueue::request_finished(evhtp_request_t *p_evhtp_req) {
  if (!request_in_progress) {
    s3_log(S3_LOG_DEBUG, nullptr, "Double invocation");
    return;
  }
  if (p_evhtp_req) {
    evhtp_unset_all_hooks(&p_evhtp_req->hooks);
  }
  request_in_progress = false;
  const bool is_succeed = is_request_succeed(p_evhtp_req);

  if (is_succeed) {
    assert(!msg_queue.empty());
    msg_queue.pop();

    n_err = 0;
  } else {
    s3_log(S3_LOG_ERROR, nullptr, "Sending of a message failed");
  }
  if (msg_queue.empty()) {
    s3_log(S3_LOG_DEBUG, nullptr, "None messages in the queue");
    return;
  }
  s3_log(S3_LOG_DEBUG, nullptr, "Schedule next message posting");

  if (!p_event) {
    p_event = event_new(p_evbase, -1, 0, on_schedule_cb, this);

    if (!p_event) {
      s3_log(S3_LOG_ERROR, nullptr, "event_new() failed");
      ++n_err;
      return;
    }
  }
  if (is_succeed) {
    event_active(p_event, 0, 1);
  } else if (event_add(p_event, &tv)) {
    s3_log(S3_LOG_ERROR, nullptr, "event_add() failed");
    ++n_err;
  }
}

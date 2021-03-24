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

#include <unistd.h>
#include <sys/socket.h>

#include <openssl/ssl.h>

#include "evhtp_wrapper.h"

void EvhtpWrapper::http_request_pause(evhtp_request_t *request) {
  evhtp_request_pause(request);
}

void EvhtpWrapper::http_request_resume(evhtp_request_t *request) {
  evhtp_request_resume(request);
}

void EvhtpWrapper::http_cancel_request(evhtp_request_t *request) {
  // evhttp_cancel_request(request);
}

evhtp_proto EvhtpWrapper::http_request_get_proto(evhtp_request_t *request) {
  return evhtp_request_get_proto(request);
}

int EvhtpWrapper::http_kvs_for_each(evhtp_kvs_t *kvs, evhtp_kvs_iterator cb,
                                    void *arg) {
  return evhtp_kvs_for_each(kvs, cb, arg);
}

const char *EvhtpWrapper::http_header_find(evhtp_headers_t *headers,
                                           const char *key) {
  return evhtp_header_find(headers, key);
}

void EvhtpWrapper::http_headers_add_header(evhtp_headers_t *headers,
                                           evhtp_header_t *header) {
  evhtp_headers_add_header(headers, header);
}

evhtp_header_t *EvhtpWrapper::http_header_new(const char *key, const char *val,
                                              char kalloc, char valloc) {
  return evhtp_header_new(key, val, kalloc, valloc);
}

const char *EvhtpWrapper::http_kv_find(evhtp_kvs_t *kvs, const char *key) {
  return evhtp_kv_find(kvs, key);
}

evhtp_kv_t *EvhtpWrapper::http_kvs_find_kv(evhtp_kvs_t *kvs, const char *key) {
  return evhtp_kvs_find_kv(kvs, key);
}

void EvhtpWrapper::http_send_reply(evhtp_request_t *request, evhtp_res code) {
  evhtp_send_reply(request, code);
}

void EvhtpWrapper::http_send_reply_start(evhtp_request_t *request,
                                         evhtp_res code) {
  evhtp_send_reply_start(request, code);
}

void EvhtpWrapper::http_send_reply_body(evhtp_request_t *request,
                                        evbuf_t *buf) {
  evhtp_send_reply_body(request, buf);
}

void EvhtpWrapper::http_send_reply_end(evhtp_request_t *request) {
  evhtp_send_reply_end(request);
}

static bool conn_has_data_for_writing(evhtp_connection_t *p_conn) noexcept {
  struct evbuffer *p_evbuf = bufferevent_get_output(p_conn->bev);

  return evbuffer_get_length(p_evbuf) > 0;
}

static void shutdown_conn(evhtp_connection_t *p_conn) {
  if (p_conn->request) {
    evhtp_unset_all_hooks(&p_conn->request->hooks);
  }
  evhtp_unset_all_hooks(&p_conn->hooks);

  if (p_conn->ssl) {
    SSL_shutdown(p_conn->ssl);
  } else {
    shutdown(p_conn->sock, SHUT_WR);
  }
}

static evhtp_res hook_write_cb(evhtp_connection_t *p_conn, void *) noexcept {

  if (!conn_has_data_for_writing(p_conn)) {
    shutdown_conn(p_conn);
  }
  return EVHTP_RES_OK;
}

void EvhtpWrapper::close_connection_after_writing(evhtp_connection_t *p_conn) {

  if (conn_has_data_for_writing(p_conn)) {

    evhtp_set_hook(&p_conn->hooks, evhtp_hook_on_write,
                   (evhtp_hook)hook_write_cb, NULL);
  } else {
    shutdown_conn(p_conn);
  }
}

// Libevent wrappers
size_t EvhtpWrapper::evbuffer_get_length(const struct evbuffer *buf) {
  return ::evbuffer_get_length(buf);
}

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

#ifndef __S3_SERVER_EVHTP_WRAPPER_H__
#define __S3_SERVER_EVHTP_WRAPPER_H__

#include "s3_common.h"

EXTERN_C_BLOCK_BEGIN

/* libevhtp */
#include <evhtp.h>

EXTERN_C_BLOCK_END

// A wrapper class for libevhtp functions so that we are able to mock
// c functions in tests. For Prod (non-test) this will just forward the calls.
class EvhtpInterface {
 public:
  virtual ~EvhtpInterface() {}
  virtual void http_request_pause(evhtp_request_t *request) = 0;
  virtual void http_request_resume(evhtp_request_t *request) = 0;
  virtual evhtp_proto http_request_get_proto(evhtp_request_t *request) = 0;

  virtual int http_kvs_for_each(evhtp_kvs_t *kvs, evhtp_kvs_iterator cb,
                                void *arg) = 0;
  virtual const char *http_header_find(evhtp_headers_t *headers,
                                       const char *key) = 0;

  virtual void http_headers_add_header(evhtp_headers_t *headers,
                                       evhtp_header_t *header) = 0;
  virtual evhtp_header_t *http_header_new(const char *key, const char *val,
                                          char kalloc, char valloc) = 0;
  virtual const char *http_kv_find(evhtp_kvs_t *kvs, const char *key) = 0;
  virtual evhtp_kv_t *http_kvs_find_kv(evhtp_kvs_t *kvs, const char *key) = 0;

  virtual void http_send_reply(evhtp_request_t *request, evhtp_res code) = 0;
  virtual void http_send_reply_start(evhtp_request_t *request,
                                     evhtp_res code) = 0;
  virtual void http_send_reply_body(evhtp_request_t *request, evbuf_t *buf) = 0;
  virtual void http_send_reply_end(evhtp_request_t *request) = 0;

  // Libevent wrappers
  virtual size_t evbuffer_get_length(const struct evbuffer *buf) = 0;
};

class EvhtpWrapper : public EvhtpInterface {
 public:
  void http_request_pause(evhtp_request_t *request);
  void http_request_resume(evhtp_request_t *request);
  evhtp_proto http_request_get_proto(evhtp_request_t *request);

  int http_kvs_for_each(evhtp_kvs_t *kvs, evhtp_kvs_iterator cb, void *arg);
  const char *http_header_find(evhtp_headers_t *headers, const char *key);

  void http_headers_add_header(evhtp_headers_t *headers,
                               evhtp_header_t *header);
  evhtp_header_t *http_header_new(const char *key, const char *val, char kalloc,
                                  char valloc);
  const char *http_kv_find(evhtp_kvs_t *kvs, const char *key);
  evhtp_kv_t *http_kvs_find_kv(evhtp_kvs_t *kvs, const char *key);

  void http_send_reply(evhtp_request_t *request, evhtp_res code);
  void http_send_reply_start(evhtp_request_t *request, evhtp_res code);
  void http_send_reply_body(evhtp_request_t *request, evbuf_t *buf);
  void http_send_reply_end(evhtp_request_t *request);

  // Libevent wrappers
  size_t evbuffer_get_length(const struct evbuffer *buf);
};

#endif

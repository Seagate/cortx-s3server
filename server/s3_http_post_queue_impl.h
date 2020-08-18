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

#ifndef __S3_SERVER_S3_HTTP_POST_QUEUE_IMPL_H__
#define __S3_SERVER_S3_HTTP_POST_QUEUE_IMPL_H__

#include <cstdint>

#include <functional>
#include <map>
#include <memory>
#include <queue>

#include <gtest/gtest_prod.h>
#include <evhtp.h>

#include "s3_http_post_queue.h"

class S3HttpPostEngine {
 public:
  virtual ~S3HttpPostEngine();
  virtual bool set_callbacks(std::function<void(void)> on_success,
                             std::function<void(void)> on_fail) = 0;
  virtual bool post(const std::string &msg) = 0;
};

class S3HttpPostQueueImpl : public S3HttpPostQueue {
 public:
  S3HttpPostQueueImpl(S3HttpPostEngine *);
  ~S3HttpPostQueueImpl();

  bool post(std::string msg) override;

 protected:
  virtual void on_msg_sent();
  virtual void on_error();

 private:
  void send_front();

  enum : unsigned {
    MAX_ERR = 100
  };
  enum : unsigned {
    MAX_MSG_IN_QUEUE = 1024
  };
  unsigned n_err = 0;
  bool msg_in_progress = false;

  std::queue<std::string> msg_queue;
  std::unique_ptr<S3HttpPostEngine> http_post_engine;

  FRIEND_TEST(S3HttpPostQueueTest, Basic);
  FRIEND_TEST(S3HttpPostQueueTest, InProgress);
  FRIEND_TEST(S3HttpPostQueueTest, ErrorCount);
  FRIEND_TEST(S3HttpPostQueueTest, Thresholds);
};

class S3HttpPostEngineImpl : public S3HttpPostEngine {
  typedef struct evdns_base evdns_base_t;

 public:
  S3HttpPostEngineImpl(evbase_t *p_evbase, std::string s_host, uint16_t port,
                       std::string path,
                       std::map<std::string, std::string> headers);
  ~S3HttpPostEngineImpl();

  // Next 2 functions should return FALSE if any of functors is "bad"
  bool set_callbacks(std::function<void(void)> on_success,
                     std::function<void(void)> on_fail) override;
  // In additional to previous comment the function should return FALSE
  // if msg is empty or the function is called before a callback for the
  // previous invocation is called.
  bool post(const std::string &msg) override;

 private:
  void call_error_callback();
  bool connect();
  bool prepare_scheduling();
  void request_finished(evhtp_request_t *);
  bool send_request(const std::string &);

  static evhtp_res on_conn_err_cb(evhtp_connection_t *, evhtp_error_flags,
                                  void *) noexcept;
  static evhtp_res on_conn_fini_cb(evhtp_connection_t *, void *) noexcept;

  static evhtp_res on_headers_start_cb(evhtp_request_t *, void *) noexcept;
  static evhtp_res on_header_cb(evhtp_request_t *, evhtp_header_t *,
                                void *) noexcept;
  static evhtp_res on_headers_cb(evhtp_request_t *, evhtp_headers_t *,
                                 void *) noexcept;
  static evhtp_res on_response_data_cb(evhtp_request_t *, evbuf_t *,
                                       void *) noexcept;

  static void on_schedule_cb(evutil_socket_t, short, void *) noexcept;

  evbase_t *const p_evbase;
  const std::string s_host;
  const uint16_t port;
  const std::string path;

  evdns_base_t *p_evdns_base = nullptr;
  evhtp_connection_t *p_conn = nullptr;
  event_t *p_event = nullptr;
  bool request_in_progress = false;
  bool f_error = false;
  size_t response_content_length = 0;
  size_t n_read = 0;

  std::function<void(void)> on_success;
  std::function<void(void)> on_fail;

  std::string msg_to_send;
  std::map<std::string, std::string> headers;
};

#endif  // __S3_SERVER_S3_HTTP_POST_QUEUE_IMPL_H__

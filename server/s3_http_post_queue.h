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

#ifndef __S3_SERVER_S3_HTTP_POST_QUEUE_H__
#define __S3_SERVER_S3_HTTP_POST_QUEUE_H__

#include <cstdint>

#include <map>
#include <memory>
#include <string>
#include <queue>

#include <evhtp.h>

class S3HttpPostQueue {
 public:
  S3HttpPostQueue(evbase_t *p_evbase, std::string s_ipa, uint16_t port = 80,
                  std::string path = "/");
  ~S3HttpPostQueue();

  S3HttpPostQueue(const S3HttpPostQueue &) = delete;
  S3HttpPostQueue &operator=(const S3HttpPostQueue &) = delete;

  void add_header(std::string name, std::string value);
  bool post(std::string msg);

 private:
  bool connect();
  bool post_msg_from_queue();
  void request_finished(evhtp_request_t *);
  void unset_all_hooks();

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
  const std::string s_ipa;
  const uint16_t port;
  const std::string path;

  evhtp_connection_t *p_conn = nullptr;
  event_t *p_event = nullptr;
  bool request_in_progress = false;
  size_t response_content_length;
  size_t n_read;

  enum {
    MAX_ERR = 100
  };
  unsigned n_err = 0;

  std::map<std::string, std::string> headers;

  enum {
    MAX_MSG_IN_QUEUE = 1024
  };
  std::queue<std::string> msg_queue;
};

#endif  // __S3_SERVER_S3_HTTP_POST_QUEUE_H__

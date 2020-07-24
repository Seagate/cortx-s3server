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

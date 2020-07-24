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
 * Original creation date: 22-Jul-2020
 */

#ifndef __S3_SERVER_S3_HTTP_POST_QUEUE_IMPL_H__
#define __S3_SERVER_S3_HTTP_POST_QUEUE_IMPL_H__

#include <cstdint>

#include <functional>
#include <map>
#include <memory>
#include <queue>

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

  enum {
    MAX_ERR = 100
  };
  enum {
    MAX_MSG_IN_QUEUE = 1024
  };
  unsigned n_err = 0;
  bool msg_in_progress = false;

  std::queue<std::string> msg_queue;
  std::unique_ptr<S3HttpPostEngine> http_post_engine;
};

class S3HttpPostEngineImpl : public S3HttpPostEngine {
 public:
  S3HttpPostEngineImpl(evbase_t *p_evbase, std::string s_ipa, uint16_t port,
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
  const std::string s_ipa;
  const uint16_t port;
  const std::string path;

  evhtp_connection_t *p_conn = nullptr;
  event_t *p_event = nullptr;
  bool request_in_progress = false;
  bool f_error = false;
  size_t response_content_length;
  size_t n_read;

  std::function<void(void)> on_success;
  std::function<void(void)> on_fail;

  std::string msg_to_send;
  std::map<std::string, std::string> headers;
};

#endif  // __S3_SERVER_S3_HTTP_POST_QUEUE_IMPL_H__

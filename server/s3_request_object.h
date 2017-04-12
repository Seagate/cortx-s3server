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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __S3_SERVER_S3_REQUEST_OBJECT_H__
#define __S3_SERVER_S3_REQUEST_OBJECT_H__

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

/* libevhtp */
#include <gtest/gtest_prod.h>
#include "evhtp_wrapper.h"

#include "s3_async_buffer_opt.h"
#include "s3_chunk_payload_parser.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_timer.h"
#include "s3_uuid.h"

enum class S3HttpVerb {
  HEAD = htp_method_HEAD,
  GET = htp_method_GET,
  PUT = htp_method_PUT,
  DELETE = htp_method_DELETE,
  POST = htp_method_POST
};

enum class S3RequestError {
  None,
  EntityTooSmall,
  InvalidPartOrder,
  InvalidChunkFormat
};

extern "C" int consume_header(evhtp_kv_t* kvobj, void* arg);

class S3AsyncBufferOptContainerFactory;

class S3RequestObject {
  evhtp_request_t* ev_req;
  std::string bucket_name;
  std::string object_name;
  // This info is fetched from auth service.
  std::string user_name;
  std::string user_id;  // Unique
  std::string account_name;
  std::string account_id;  // Unique

  std::string request_id;

  bool is_paused;  // indicates whether request is explicitly paused

  size_t notify_read_watermark;  // notification sent when available data is
                                 // more than this.
  size_t pending_in_flight;      // Total data yet to consume by observer.
  size_t total_bytes_received;
  std::shared_ptr<S3AsyncBufferOptContainer> buffered_input;

  std::function<void()> incoming_data_callback;

  std::unique_ptr<EvhtpInterface> evhtp_obj;

  S3Timer request_timer;

  bool is_client_connected;

  bool is_chunked_upload;
  S3ChunkPayloadParser chunk_parser;
  S3ApiType s3_api_type;
  std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buffer_factory;

  virtual void set_full_path(const char* full_path);
  virtual void set_file_name(const char* file_name);
  virtual void set_query_params(const char* query_params);

 public:
  S3RequestObject(evhtp_request_t* req, EvhtpInterface* evhtp_obj_ptr,
                  std::shared_ptr<S3AsyncBufferOptContainerFactory>
                      async_buf_factory = nullptr);
  virtual ~S3RequestObject();

  // Broken into helper function primarily to allow initialisations after faking
  // data.
  void initialise();

  virtual const char* c_get_uri_query();
  virtual S3HttpVerb http_verb();
  virtual const char* c_get_full_path();
  virtual const char* c_get_full_encoded_path();
  const char* c_get_file_name();
  void set_api_type(S3ApiType apitype);
  virtual S3ApiType get_api_type();

 protected:
  // protected so mocks can override
  std::map<std::string, std::string> in_headers_copy;
  bool in_headers_copied;

 private:
  std::string full_request_body;
  std::string full_path_decoded_uri;
  std::string file_path_decoded_uri;
  std::string query_raw_decoded_uri;
  S3RequestError request_error;

 public:
  virtual std::map<std::string, std::string>& get_in_headers_copy();
  friend int consume_header(evhtp_kv_t* kvobj, void* arg);

  virtual std::string get_header_value(std::string key);
  virtual std::string get_host_header();

  // returns x-amz-decoded-content-length OR Content-Length
  // Always prefer get_data_length*() version since it takes
  // care of both above headers (chunked and non-chunked cases)
  virtual size_t get_data_length();
  virtual std::string get_data_length_str();

  virtual size_t get_content_length();
  virtual std::string get_content_length_str();

  virtual std::string& get_full_body_content_as_string();

  virtual std::string get_query_string_value(std::string key);
  virtual bool has_query_param_key(std::string key);

  // xxx Remove this soon
  struct evbuffer* buffer_in() {
    return ev_req->buffer_in;
  }

  virtual void set_out_header_value(std::string key, std::string value);

  // Operation params.
  std::string get_object_uri();

  virtual void set_bucket_name(const std::string& name);
  const std::string& get_bucket_name();
  virtual void set_object_name(const std::string& name);
  const std::string& get_object_name();

  void set_user_name(const std::string& name);
  const std::string& get_user_name();
  void set_user_id(const std::string& id);
  const std::string& get_user_id();

  void set_account_name(const std::string& name);
  const std::string& get_account_name();
  void set_account_id(const std::string& id);
  const std::string& get_account_id();

  const std::string& get_request_id();

  S3RequestError get_request_error() { return request_error; }

  void set_request_error(S3RequestError req_error) {
    request_error = req_error;
  }

  /* INFO: https://github.com/ellzey/libevhtp/issues/93
     Pause and resume will essentially stop and start attempting to read from
     the client socket.
     But also has some extra logic around it. Since the parser is a state
     machine, you can get functions executed as data is passed to it. But what
     if you wanted to temporarily stop that input processing? Well you have to
     save the state on the current input, then turn off the reading.

     You can do all the normal things you can do while the request is paused,
     add output data, headers, etc. But it isn't until you use the resume
     function will these actions actually happen on the socket. When resume is
     called, it does an event_active(), which in turn goes back to the original
     reader function.
  */
  // We specifically use this when auth is in-progress so that
  // we dont flood with data coming from socket in user buffers.
  virtual void pause() {
    if (!is_paused) {
      s3_log(S3_LOG_DEBUG, "Pausing the request for sock %d...\n",
             ev_req->conn->sock);
      evhtp_obj->http_request_pause(ev_req);
      is_paused = true;
    }
  }

  virtual void resume() {
    if (is_paused) {
      s3_log(S3_LOG_DEBUG, "Resuming the request for sock %d...\n",
             ev_req->conn->sock);
      evhtp_obj->http_request_resume(ev_req);
      is_paused = false;
    }
  }

  void client_has_disconnected() { is_client_connected = false; }

  bool client_connected() { return is_client_connected; }

  bool is_chunked() { return is_chunked_upload; }

  // pass thru functions
  virtual bool is_chunk_detail_ready() {
    return chunk_parser.is_chunk_detail_ready();
  }

  virtual S3ChunkDetail pop_chunk_detail() {
    return chunk_parser.pop_chunk_detail();
  }

  // Streaming
  // Note: Call this only if request object has to still receive from socket
  // Setup listeners for input data (data coming from s3 clients)
  // notify_on_size: notify when we have atleast 'notify_read_watermark' bytes
  // of data available of if its last payload.

  // Notifications are sent on socket read events. In case we already have full
  // body
  // in first payload, notify will just remember it and caller should grab the
  // buffers
  // without a listener.
  virtual void listen_for_incoming_data(std::function<void()> callback,
                                        size_t notify_on_size) {
    notify_read_watermark = notify_on_size;
    incoming_data_callback = callback;
    resume();  // resume reading if it was paused
  }

  void notify_incoming_data(evbuf_t* buf);

  // Check whether we already have (read) the entire body.
  virtual bool has_all_body_content() { return (pending_in_flight == 0); }

  std::shared_ptr<S3AsyncBufferOptContainer> get_buffered_input() {
    return buffered_input;
  }

  // Response Helpers
 private:
  struct evbuffer* reply_buffer;

 public:
  virtual void send_response(int code, std::string body = "");
  virtual void send_reply_start(int code);
  virtual void send_reply_body(char* data, int length);
  virtual void send_reply_end();

  void respond_error(std::string error_code,
                     const std::map<std::string, std::string>& headers =
                         std::map<std::string, std::string>());

  void respond_unsupported_api();
  virtual void respond_retry_after(int retry_after_in_secs = 1);

  FRIEND_TEST(S3MockAuthClientCheckTest, CheckAuth);
  FRIEND_TEST(S3RequestObjectTest, ReturnsValidUriPaths);
  FRIEND_TEST(S3RequestObjectTest, ReturnsValidRawQuery);
};

#endif

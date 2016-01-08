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

#ifndef __MERO_FE_S3_SERVER_S3_REQUEST_OBJECT_H__
#define __MERO_FE_S3_SERVER_S3_REQUEST_OBJECT_H__

#include <string>
#include <memory>
#include <map>
#include <set>
#include <tuple>
#include <vector>

/* libevhtp */
#include "evhtp_wrapper.h"
#include <gtest/gtest_prod.h>

#include "s3_async_buffer.h"
#include "s3_timer.h"
#include "s3_perf_logger.h"

enum class S3HttpVerb {
  HEAD = htp_method_HEAD,
  GET = htp_method_GET,
  PUT = htp_method_PUT,
  DELETE = htp_method_DELETE,
  POST = htp_method_POST
};

extern "C" int consume_header(evhtp_kv_t * kvobj, void * arg);

class S3RequestObject {
  evhtp_request_t *ev_req;
  std::string bucket_name;
  std::string object_name;
  // This info is fetched from auth service.
  std::string user_name;
  std::string user_id;  // Unique
  std::string account_name;
  std::string account_id;  // Unique

  std::string request_id;

  bool is_paused;  // indicates whether request is explicitly paused

  size_t notify_read_watermark;  // notification sent when available data is more than this.
  size_t pending_in_flight;  // Total data yet to consume by observer.
  size_t total_bytes_received;
  S3AsyncBufferContainer buffered_input;

  std::function<void()> incoming_data_callback;

  std::unique_ptr<EvhtpInterface> evhtp_obj;

  S3Timer request_timer;

public:
  S3RequestObject(evhtp_request_t *req, EvhtpInterface *evhtp_obj_ptr);
  virtual ~S3RequestObject();

  // Broken into helper function primarily to allow initialisations after faking data.
  void initialise();

  virtual struct event_base* get_evbase() {
    if(this->ev_req != NULL && this->ev_req->htp != NULL) {
      return this->ev_req->htp->evbase;
    } else {
      return NULL;
    }
  }

  virtual evhtp_request_t * get_request() {
    return this->ev_req;
  }

  virtual const char * c_get_uri_query();
  virtual S3HttpVerb http_verb();

  virtual const char* c_get_full_path();

  char * c_get_file_name();
private:
  std::map<std::string, std::string> in_headers_copy;
  bool in_headers_copied;
  std::string full_request_body;
public:
  std::map<std::string, std::string> get_in_headers_copy();
  friend int consume_header(evhtp_kv_t * kvobj, void * arg);

  std::string get_header_value(std::string key);
  virtual std::string get_host_header();
  size_t get_content_length();
  std::string& get_full_body_content_as_string();

  std::string get_query_string_value(std::string key);

  virtual bool has_query_param_key(std::string key);

  // xxx Remove this soon
  struct evbuffer* buffer_in() {
    return ev_req->buffer_in;
  }

  void set_out_header_value(std::string key, std::string value);

  // Operation params.
  std::string get_object_uri();

  virtual void set_bucket_name(std::string& name);
  std::string& get_bucket_name();
  virtual void set_object_name(std::string& name);
  std::string& get_object_name();

  void set_user_name(std::string& name);
  std::string& get_user_name();
  void set_user_id(std::string& id);
  std::string& get_user_id();

  void set_account_name(std::string& name);
  std::string& get_account_name();
  void set_account_id(std::string& id);
  std::string& get_account_id();

  std::string& get_request_id();

  /* INFO: https://github.com/ellzey/libevhtp/issues/93
     Pause and resume will essentially stop and start attempting to read from the client socket.
     But also has some extra logic around it. Since the parser is a state machine, you can get functions executed as data is passed to it. But what if you wanted to temporarily stop that input processing? Well you have to save the state on the current input, then turn off the reading.

     You can do all the normal things you can do while the request is paused, add output data, headers, etc. But it isn't until you use the resume function will these actions actually happen on the socket. When resume is called, it does an event_active(), which in turn goes back to the original reader function.
  */
  // We specifically use this when auth is in-progress so that
  // we dont flood with data coming from socket in user buffers.
  void pause() {
    if (!is_paused) {
      printf("Pausing the request for sock %d.....\n", ev_req->conn->sock);
      evhtp_obj->http_request_pause(ev_req);
      is_paused = true;
    }
  }

  void resume() {
    if (is_paused) {
      printf("Resuming the request for sock %d.....\n", ev_req->conn->sock);
      evhtp_obj->http_request_resume(ev_req);
      is_paused = false;
    }
  }

  // Streaming
  // Note: Call this only if request object has to still receive from socket
  // Setup listeners for input data (data coming from s3 clients)
  // notify_on_size: notify when we have atleast 'notify_read_watermark' bytes
  // of data available of if its last payload.

  // Notifications are sent on socket read events. In case we already have full body
  // in first payload, notify will just remember it and caller should grab the buffers
  // without a listener.
  void listen_for_incoming_data(
    std::function<void()> callback, size_t notify_on_size) {
      notify_read_watermark = notify_on_size;
      incoming_data_callback = callback;
  }

  void notify_incoming_data(evbuf_t * buf) {
    // Keep buffering till someone starts listening.
    size_t bytes_received = evhtp_obj->evbuffer_get_length(buf);
    printf("Buffering data to be consumed.....%zu\n", bytes_received);
    S3Timer buffering_timer;
    buffering_timer.start();

    buffered_input.add_content(buf);
    pending_in_flight -= bytes_received;
    if (pending_in_flight == 0) {
      printf("Buffering complete for data to be consumed.....\n");
      buffered_input.freeze();
    }
    buffering_timer.stop();
    LOG_PERF(("total_buffering_time_" + std::to_string(bytes_received) + "_bytes_ns").c_str(),
      buffering_timer.elapsed_time_in_nanosec());

    if ( incoming_data_callback &&
         ((buffered_input.length() >= notify_read_watermark) || (pending_in_flight == 0)) ) {
      printf("Sending data to be consumed.....\n");
      incoming_data_callback();
    }
  }

  // Check whether we already have (read) the entire body.
  size_t has_all_body_content() {
    return (pending_in_flight == 0);
  }

  S3AsyncBufferContainer& get_buffered_input() {
    return buffered_input;
  }

  // Response Helpers
private:
  struct evbuffer* reply_buffer;

public:
  void send_response(int code, std::string body = "");
  void send_reply_start(int code);
  void send_reply_body(char *data, int length);
  void send_reply_end();
  void respond_unsupported_api();

  FRIEND_TEST(S3MockAuthClientCheckTest, CheckAuth);
};

#endif

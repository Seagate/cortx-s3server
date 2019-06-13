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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include <evhttp.h>
#include <string>
#include <algorithm>

#include "s3_error_codes.h"
#include "s3_factory.h"
#include "s3_option.h"
#include "s3_common_utilities.h"
#include "s3_request_object.h"
#include "s3_stats.h"

extern S3Option* g_option_instance;

// evhttp Helpers
/* evhtp_kvs_iterator */
extern "C" int consume_header(evhtp_kv_t* kvobj, void* arg) {
  S3RequestObject* request = (S3RequestObject*)arg;
  request->in_headers_copy[kvobj->key] = kvobj->val ? kvobj->val : "";
  return 0;
}

extern "C" int consume_query_parameters(evhtp_kv_t* kvobj, void* arg) {
  S3RequestObject* request = (S3RequestObject*)arg;
  if (request && kvobj) {
    if (kvobj->val) {
      char* decoded_str = evhttp_uridecode(kvobj->val, 1, NULL);
      request->in_query_params_copy[kvobj->key] = decoded_str;
      free(decoded_str);
    } else {
      request->in_query_params_copy[kvobj->key] = "";
    }
  }
  return 0;
}

extern "C" void s3_client_read_timeout_cb(evutil_socket_t fd, short event,
                                          void* arg) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  S3RequestObject* request = static_cast<S3RequestObject*>(arg);
  if (request == NULL) {
    return;
  }
  request->get_buffered_input()->freeze();
  s3_log(S3_LOG_WARN, request->get_request_id().c_str(),
         "Read timeout Occured\n");
  request->free_client_read_timer();
  request->trigger_client_read_timeout_callback();
  return;
}

S3RequestObject::S3RequestObject(
    evhtp_request_t* req, EvhtpInterface* evhtp_obj_ptr,
    std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buf_factory,
    EventInterface* event_obj_ptr)
    : ev_req(req),
      http_method(S3HttpVerb::UNKNOWN),
      is_paused(false),
      notify_read_watermark(0),
      total_bytes_received(0),
      is_client_connected(true),
      s3_client_read_timedout(false),
      is_chunked_upload(false),
      s3_api_type(S3ApiType::unsupported),
      in_headers_copied(false),
      in_query_params_copied(false),
      reply_buffer(NULL) {
  S3Uuid uuid;
  request_id = uuid.get_string_uuid();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  if (async_buf_factory) {
    async_buffer_factory = async_buf_factory;
  } else {
    async_buffer_factory = std::make_shared<S3AsyncBufferOptContainerFactory>();
  }

  if (event_obj_ptr) {
    event_obj.reset(event_obj_ptr);
  } else {
    event_obj = std::unique_ptr<EventWrapper>(new EventWrapper());
  }

  buffered_input = async_buffer_factory->create_async_buffer(
      g_option_instance->get_libevent_pool_buffer_size());

  request_timer.start();
  turn_around_time.start();
  bucket_name = object_name = user_name = user_id = account_name = account_id =
      "";
  object_size = 0;
  // For auth disabled, use some dummy user.
  if (g_option_instance->is_auth_disabled()) {
    account_name = "s3_test";
    account_id = "12345";
    user_name = "tester";
    user_id = "123";
    audit_log_obj.set_authentication_type("QueryString");
  } else {
    audit_log_obj.set_authentication_type("AuthHeader");
  }

  request_error = S3RequestError::None;
  client_read_timer_event = NULL;
  evhtp_obj.reset(evhtp_obj_ptr);
  if (ev_req != NULL) {
    http_method = (S3HttpVerb)ev_req->method;
    if (ev_req->uri != NULL) {
      if (ev_req->uri->path != NULL) {
        char* decoded_str = evhttp_uridecode(ev_req->uri->path->full, 1, NULL);
        full_path_decoded_uri = decoded_str;
        free(decoded_str);
        if (ev_req->uri->path->file != NULL) {
          char* decoded_str =
              evhttp_uridecode(ev_req->uri->path->file, 1, NULL);
          file_path_decoded_uri = decoded_str;
          free(decoded_str);
        }
        if (evhtp_obj->http_request_get_proto(ev_req) ==
            evhtp_proto::EVHTP_PROTO_10) {
          http_version = "HTTP/1.0";
        } else if (evhtp_obj->http_request_get_proto(ev_req) ==
                   evhtp_proto::EVHTP_PROTO_11) {
          http_version = "HTTP/1.1";
        }
      }
      if (ev_req->uri->query_raw != NULL) {
        char* decoded_str =
            evhttp_uridecode((const char*)ev_req->uri->query_raw, 1, NULL);
        query_raw_decoded_uri = decoded_str;
        free(decoded_str);
      }
    }
  } else {
    s3_log(S3_LOG_WARN, request_id, "s3 client disconnected state.\n");
  }
}

void S3RequestObject::initialise() {
  s3_log(S3_LOG_DEBUG, request_id, "Initializing the request.\n");
  audit_log_obj.set_time_of_request_arrival();
  if (get_header_value("x-amz-content-sha256") ==
      "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
    is_chunked_upload = true;
  }
  pending_in_flight = get_data_length();
  chunk_parser.setup_content_length(pending_in_flight);
  if (pending_in_flight == 0) {
    // We are not expecting any payload.
    buffered_input->freeze();
  }
}

S3RequestObject::~S3RequestObject() {
  s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");

  if (ev_req) {
    ev_req->cbarg = NULL;
    ev_req = NULL;
  }
  if (reply_buffer != NULL) {
    evbuffer_free(reply_buffer);
    reply_buffer = NULL;
  }
  free_client_read_timer();
}

const std::map<std::string, std::string, compare>&
S3RequestObject::get_query_parameters() {
  if (!in_query_params_copied) {
    if (client_connected() && (ev_req != NULL) && (ev_req->uri != NULL) &&
        (ev_req->uri->query != NULL)) {
      evhtp_obj->http_kvs_for_each(ev_req->uri->query, consume_query_parameters,
                                   this);
      in_query_params_copied = true;
    } else {
      s3_log(S3_LOG_WARN, request_id,
             "s3 client disconnected state or ev_req(NULL).\n");
    }
  }
  return in_query_params_copy;
}

S3HttpVerb S3RequestObject::http_verb() { return http_method; }

const char* S3RequestObject::get_http_verb_str(S3HttpVerb method) {
  return htparser_get_methodstr_m((htp_method)method);
}

void S3RequestObject::set_full_path(const char* full_path) {
  full_path_decoded_uri = full_path;
}

void S3RequestObject::set_start_client_request_read_timeout() {
  // Set the timer event, so that if data doesn't arrive within a
  // configured timeframe, then send error to client.
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (s3_client_read_timedout) {
    return;
  }
  struct timeval tv;
  tv.tv_usec = 0;
  tv.tv_sec = g_option_instance->get_client_req_read_timeout_secs();
  if (client_read_timer_event == NULL) {
    // Create timer event
    client_read_timer_event =
        event_obj->new_event(g_option_instance->get_eventbase(), -1, 0,
                             s3_client_read_timeout_cb, (void*)this);
  }
  s3_log(S3_LOG_DEBUG, request_id,
         "Setting client read timeout to %ld seconds\n", tv.tv_sec);
  event_obj->add_event(client_read_timer_event, &tv);
  return;
}

void S3RequestObject::stop_client_read_timer() {
  if (client_read_timer_event != NULL) {
    if (event_obj->pending_event(client_read_timer_event, EV_TIMEOUT)) {
      s3_log(S3_LOG_DEBUG, request_id, "Deleting client read timer\n");
      event_obj->del_event(client_read_timer_event);
    }
  }
  return;
}

void S3RequestObject::restart_client_read_timer() {
  // delete any pending timer
  stop_client_read_timer();
  // Set new timer
  set_start_client_request_read_timeout();
  return;
}

void S3RequestObject::free_client_read_timer() {
  if (client_read_timer_event != NULL) {
    s3_log(S3_LOG_DEBUG, request_id.c_str(), "Calling event free\n");
    event_obj->free_event(client_read_timer_event);
    client_read_timer_event = NULL;
  }
}

void S3RequestObject::trigger_client_read_timeout_callback() {
  s3_client_read_timedout = true;
  if (client_read_timeout_callback) {
    client_read_timeout_callback();
  }
}

const char* S3RequestObject::c_get_full_path() {
  return full_path_decoded_uri.c_str();
}

const char* S3RequestObject::c_get_full_encoded_path() {
  if (client_connected()) {
    if ((ev_req != NULL) && (ev_req->uri != NULL) &&
        (ev_req->uri->path != NULL)) {
      return ev_req->uri->path->full;
    }
  } else {
    s3_log(S3_LOG_WARN, request_id, "s3 client disconnected state.\n");
  }
  return NULL;
}

void S3RequestObject::set_file_name(const char* file_name) {
  file_path_decoded_uri = file_name;
}

const char* S3RequestObject::c_get_file_name() {
  return file_path_decoded_uri.c_str();
}

void S3RequestObject::set_query_params(const char* query_params) {
  query_raw_decoded_uri = query_params;
}

const char* S3RequestObject::c_get_uri_query() {
  return query_raw_decoded_uri.c_str();
}

std::map<std::string, std::string>& S3RequestObject::get_in_headers_copy() {
  if (!in_headers_copied) {
    if (client_connected() && (ev_req != NULL)) {
      evhtp_obj->http_kvs_for_each(ev_req->headers_in, consume_header, this);
      in_headers_copied = true;
    } else {
      s3_log(S3_LOG_WARN, request_id,
             "s3 client disconnected state or ev_req(NULL).\n");
    }
  }
  return in_headers_copy;
}

std::string S3RequestObject::get_header_value(std::string key) {

  std::string val_str;
  if (!in_headers_copied) {
    get_in_headers_copy();
  }

  auto header = in_headers_copy.begin();
  while (header != in_headers_copy.end()) {
    if (!strcasecmp(header->first.c_str(), key.c_str())) {
      val_str = header->second.c_str();
      break;
    }
    header++;
  }
  return val_str;
}

bool S3RequestObject::is_valid_ipaddress(std::string& ipaddr) {
  int ret;
  unsigned char buf[sizeof(struct in6_addr)];
  size_t pos = ipaddr.find(":");

  if (pos != std::string::npos) {
    // Its IPV6
    ret = inet_pton(AF_INET6, ipaddr.c_str(), buf);
  } else {
    // Its IPV4
    ret = inet_pton(AF_INET, ipaddr.c_str(), buf);
  }
  return ret != 0;
}

std::string S3RequestObject::get_host_header() {
  std::string host = "Host";
  return get_header_value(host);
}

std::string S3RequestObject::get_host_name() {
  size_t pos;
  std::string host_name;
  std::string original_host_header = get_host_header();
  host_name = original_host_header;
  // Host may have port along with it hostname:port, if it has then strip it
  pos = original_host_header.rfind(":");
  if (pos != std::string::npos) {
    host_name = original_host_header.substr(0, pos);
    if (!is_valid_ipaddress(host_name)) {
      pos = host_name.find(":");
      if (pos == std::string::npos) {
        // This is IPV4 or DNS style name, hence return with stripped port #
        return host_name;
      }
      // IPV6, Check with original IP
      if (is_valid_ipaddress(original_host_header)) {
        // The original host header contains only IP (IPV6) so revert to it.
        host_name = original_host_header;
      }
    }
  }
  return host_name;
}

void S3RequestObject::set_out_header_value(std::string key, std::string value) {
  if (!client_connected() || ev_req == NULL) {
    s3_log(S3_LOG_WARN, request_id,
           "s3 client disconnected state or ev_req(NULL).\n");
    return;
  }
  if (out_headers_copy.find(key) == out_headers_copy.end()) {
    evhtp_obj->http_headers_add_header(
        ev_req->headers_out,
        evhtp_obj->http_header_new(key.c_str(), value.c_str(), 1, 1));
    out_headers_copy.insert(std::pair<std::string, std::string>(key, value));
  } else {
    // Single code path should not have a need to call this method for same key
    // more then once, which can also point to a code problem.
    // Example: set_out_header_value("Content-Length", "50"); <some more code>;
    // set_out_header_value("Content-Length", "5"); is usually error prone.
    assert(out_headers_copy.find(key) == out_headers_copy.end());
  }
}

std::string S3RequestObject::get_data_length_str() {
  std::string data_length_key = "x-amz-decoded-content-length";
  std::string data_length = get_header_value(data_length_key);
  data_length = S3CommonUtilities::trim(data_length);
  if (data_length.empty()) {
    // Normal request
    return get_content_length_str();
  } else {
    return data_length;
  }
}

size_t S3RequestObject::get_data_length() {
  return std::stoul(get_data_length_str());
}

std::string S3RequestObject::get_content_length_str() {
  std::string content_length = "Content-Length";
  std::string len = get_header_value(content_length);
  len = S3CommonUtilities::trim(len);
  if (len.empty()) {
    len = "0";
  }
  return len;
}

bool S3RequestObject::validate_content_length() {
  bool is_content_length_valid = true;
  unsigned long content_length = 0;

  is_content_length_valid =
      S3CommonUtilities::stoul(get_content_length_str(), content_length);

  // check content length is greater than SIZE_MAX
  if (content_length > SIZE_MAX) {
    is_content_length_valid = false;
  }
  // return if content length is not valid
  if (!is_content_length_valid) return is_content_length_valid;

  std::string data_length_key = "x-amz-decoded-content-length";
  std::string data_length = get_header_value(data_length_key);
  if (!data_length.empty()) {
    is_content_length_valid =
        S3CommonUtilities::stoul(data_length, content_length);

    // check content length is greater than SIZE_MAX
    if (content_length > SIZE_MAX) {
      is_content_length_valid = false;
    }
  }
  return is_content_length_valid;
}

size_t S3RequestObject::get_content_length() {
  // no need to handle expection here,
  // as we are handling at dispatch request
  return std::stoul(get_content_length_str());
}

std::string& S3RequestObject::get_full_body_content_as_string() {
  full_request_body = "";
  if (buffered_input->is_freezed()) {
    full_request_body = buffered_input->get_content_as_string();
  }

  return full_request_body;
}

std::string S3RequestObject::get_query_string_value(std::string key) {
  std::string val_str = "";
  if (!in_query_params_copied) {
    get_query_parameters();
  }
  auto param = in_query_params_copy.find(key);
  if (param != in_query_params_copy.end()) {
    val_str = param->second;
  }
  return val_str;
}

bool S3RequestObject::has_query_param_key(std::string key) {
  if (!in_query_params_copied) {
    get_query_parameters();
  }
  return (in_query_params_copy.find(key) != in_query_params_copy.end());
}

// Operation params.
std::string S3RequestObject::get_object_uri() {
  return bucket_name + "/" + object_name;
}

void S3RequestObject::set_bucket_name(const std::string& name) {
  bucket_name = name;
}

void S3RequestObject::set_object_size(size_t obj_size) {
  object_size = obj_size;
}
const std::string& S3RequestObject::get_bucket_name() { return bucket_name; }

void S3RequestObject::set_object_name(const std::string& name) {
  object_name = name;
}

const std::string& S3RequestObject::get_object_name() { return object_name; }

void S3RequestObject::set_user_name(const std::string& name) {
  user_name = name;
}

const std::string& S3RequestObject::get_user_name() { return user_name; }

void S3RequestObject::set_user_id(const std::string& id) { user_id = id; }

const std::string& S3RequestObject::get_user_id() { return user_id; }

void S3RequestObject::set_account_id(const std::string& id) { account_id = id; }

const std::string& S3RequestObject::get_account_id() { return account_id; }

void S3RequestObject::set_account_name(const std::string& name) {
  account_name = name;
}

const std::string& S3RequestObject::get_account_name() { return account_name; }

void S3RequestObject::set_object_acl(const std::string& acl) {
  object_acl = acl;
}

const std::string& S3RequestObject::get_object_acl() { return object_acl; }

void S3RequestObject::notify_incoming_data(evbuf_t* buf) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with buf(%p)\n", buf);
  s3_log(S3_LOG_DEBUG, request_id, "pending_in_flight (before): %zu\n",
         pending_in_flight);
  if (buf == NULL) {
    // Very unlikely
    s3_log(S3_LOG_WARN, request_id, "Exiting due to buf(NULL)\n");
    return;
  }
  if (s3_client_read_timedout) {
    s3_log(S3_LOG_WARN, request_id, "Exiting due to read timeout\n");
    return;
  }
  // Keep buffering till someone starts listening.
  size_t data_bytes_received = 0;
  S3Timer buffering_timer;
  buffering_timer.start();

  if (is_chunked_upload) {
    auto bufs = chunk_parser.run(buf);
    if (chunk_parser.get_state() == ChunkParserState::c_error) {
      s3_log(S3_LOG_DEBUG, request_id, "ChunkParserState::c_error\n");
      set_request_error(S3RequestError::InvalidChunkFormat);
    } else if (!bufs.empty()) {
      for (auto b : bufs) {
        s3_log(S3_LOG_DEBUG, request_id,
               "Adding data length to async buffer: %zu\n",
               evhtp_obj->evbuffer_get_length(b));
        data_bytes_received += evhtp_obj->evbuffer_get_length(b);
        buffered_input->add_content(
            b, (pending_in_flight - data_bytes_received) == 0);
      }
    }
  } else {
    data_bytes_received = evhtp_obj->evbuffer_get_length(buf);
    buffered_input->add_content(buf,
                                (pending_in_flight - data_bytes_received) == 0);
  }
  s3_log(S3_LOG_DEBUG, request_id, "Buffering data to be consumed: %zu\n",
         data_bytes_received);

  if (data_bytes_received > pending_in_flight) {
    s3_log(S3_LOG_ERROR, request_id, "Received too much unexpected data\n");
  }
  pending_in_flight -= data_bytes_received;
  s3_log(S3_LOG_DEBUG, request_id, "pending_in_flight (after): %zu\n",
         pending_in_flight);
  if (pending_in_flight == 0) {
    s3_log(S3_LOG_DEBUG, request_id,
           "Buffering complete for data to be consumed.\n");
    buffered_input->freeze();
  }
  buffering_timer.stop();
  LOG_PERF(("total_buffering_time_" + std::to_string(data_bytes_received) +
            "_bytes_ns")
               .c_str(),
           buffering_timer.elapsed_time_in_nanosec());
  s3_stats_timing("total_buffering_time",
                  buffering_timer.elapsed_time_in_millisec());

  if (incoming_data_callback &&
      ((buffered_input->get_content_length() >= notify_read_watermark) ||
       (pending_in_flight == 0))) {
    s3_log(S3_LOG_DEBUG, request_id, "Sending data to be consumed...\n");
    incoming_data_callback();
  }
  // Pause if we have read enough in buffers for this request,
  // and let the handlers resume when required.
  if (!incoming_data_callback && !buffered_input->is_freezed() &&
      buffered_input->get_content_length() >=
          (notify_read_watermark *
           g_option_instance->get_read_ahead_multiple())) {
    pause();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3RequestObject::send_response(int code, std::string body) {
  s3_log(S3_LOG_INFO, request_id, "Response code: [%d]\n", code);
  s3_log(S3_LOG_INFO, request_id, "Sending response as: [%s]\n", body.c_str());

  http_status = code;
  turn_around_time.stop();
  audit_log_obj.set_turn_around_time(
      turn_around_time.elapsed_time_in_millisec());

  if (code == S3HttpFailed500) {
    s3_stats_inc("internal_error_count");
  }

  if (!client_connected()) {
    s3_log(S3_LOG_WARN, request_id, "s3 client disconnected state.\n");
    request_timer.stop();
    audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
    populate_and_log_audit_info();
    return;
  }

  // If body not empty, write to response body.
  if (!body.empty()) {
    evbuffer_add(ev_req->buffer_out, body.c_str(), body.length());
    audit_log_obj.set_bytes_sent(evbuffer_get_length(ev_req->buffer_out));

  } else if (out_headers_copy.find("Content-Length") ==
             out_headers_copy.end()) {  // Content-Length was already set for
                                        // this request, which could be. case
                                        // like HEAD object request, so dont
                                        // add/update again
    set_out_header_value("Content-Length", "0");
  }
  set_out_header_value("x-amzn-RequestId", request_id);
  evhtp_obj->http_send_reply(ev_req, code);
  resume();  // attempt resume just in case some one forgot

  request_timer.stop();
  LOG_PERF("total_request_time_ms", request_timer.elapsed_time_in_millisec());
  s3_stats_timing("total_request_time",
                  request_timer.elapsed_time_in_millisec());
  audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
  populate_and_log_audit_info();
}

void S3RequestObject::send_reply_start(int code) {
  http_status = code;
  turn_around_time.stop();
  audit_log_obj.set_turn_around_time(
      turn_around_time.elapsed_time_in_millisec());
  set_out_header_value("x-amzn-RequestId", request_id);
  if (client_connected()) {
    evhtp_obj->http_send_reply_start(ev_req, code);
    reply_buffer = evbuffer_new();
  } else {
    request_timer.stop();
    LOG_PERF("total_request_time_ms", request_timer.elapsed_time_in_millisec());
    s3_stats_timing("total_request_time",
                    request_timer.elapsed_time_in_millisec());
    audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
    populate_and_log_audit_info();
  }
}

void S3RequestObject::send_reply_body(char* data, int length) {
  if (client_connected()) {
    // Setting object size for GET object request
    if (length != 0) {
      audit_log_obj.set_object_size(length);
    }
    evbuffer_add(reply_buffer, data, length);
    evhtp_obj->http_send_reply_body(ev_req, reply_buffer);
  } else {
    request_timer.stop();
    LOG_PERF("total_request_time_ms", request_timer.elapsed_time_in_millisec());
    s3_stats_timing("total_request_time",
                    request_timer.elapsed_time_in_millisec());
    audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
    populate_and_log_audit_info();
  }
}

void S3RequestObject::send_reply_end() {
  if (client_connected()) {
    evhtp_obj->http_send_reply_end(ev_req);
    request_timer.stop();
  } else {
    request_timer.stop();
  }
  LOG_PERF("total_request_time_ms", request_timer.elapsed_time_in_millisec());
  s3_stats_timing("total_request_time",
                  request_timer.elapsed_time_in_millisec());
  audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
  populate_and_log_audit_info();

  if (reply_buffer != NULL) {
    evbuffer_free(reply_buffer);
  }
  reply_buffer = NULL;
}

void S3RequestObject::set_api_type(S3ApiType api_type) {
  s3_api_type = api_type;
}

S3ApiType S3RequestObject::get_api_type() { return s3_api_type; }

void S3RequestObject::set_operation_code(S3OperationCode operation_code) {
  s3_operation_code = operation_code;
}

S3OperationCode S3RequestObject::get_operation_code() {
  return s3_operation_code;
}

void S3RequestObject::respond_error(
    std::string error_code, const std::map<std::string, std::string>& headers) {
  audit_log_obj.set_error_code(error_code);
  if (client_connected()) {
    S3Error error(error_code, get_request_id(), get_object_uri());
    std::string& response_xml = error.to_xml();
    set_out_header_value("Content-Type", "application/xml");
    set_out_header_value("Content-Length",
                         std::to_string(response_xml.length()));
    set_out_header_value("x-amzn-RequestId", request_id);
    set_out_header_value("Connection", "close");
    for (auto& header : headers) {
      set_out_header_value(header.first.c_str(), header.second.c_str());
    }
    s3_log(S3_LOG_DEBUG, request_id, "Error response to client: [%s]",
           response_xml.c_str());

    send_response(error.get_http_status_code(), response_xml);
  } else {
    request_timer.stop();
    audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
    populate_and_log_audit_info();
    s3_log(S3_LOG_WARN, request_id, "s3 client disconnected state.\n");
  }
}

void S3RequestObject::respond_unsupported_api() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  respond_error("NotImplemented");

  s3_stats_inc("unsupported_api_count");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3RequestObject::respond_retry_after(int retry_after_in_secs) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  std::map<std::string, std::string> headers;
  headers["Retry-After"] = std::to_string(retry_after_in_secs);

  respond_error("ServiceUnavailable", headers);

  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3RequestObject::populate_and_log_audit_info() {

  s3_log(S3_LOG_DEBUG, request_id, "Entering");
  std::string s3_operation_str =
      operation_code_to_audit_str(get_operation_code());

  if (!(s3_operation_str.compare("NONE") &&
        s3_operation_str.compare("UNKNOWN"))) {
    s3_operation_str.clear();
  }

  std::string audit_operation_str =
      std::string("REST.") + get_http_verb_str(http_verb()) + std::string(".") +
      api_type_to_str(get_api_type()) + s3_operation_str;

  std::string request_uri =
      get_http_verb_str(http_verb()) + std::string(" ") + full_path_decoded_uri;

  if (!query_raw_decoded_uri.empty()) {
    request_uri = request_uri + std::string("?") + query_raw_decoded_uri;
  }

  request_uri = request_uri + std::string(" ") + http_version;

  audit_log_obj.set_bucket_owner_canonical_id(get_account_id());
  audit_log_obj.set_bucket_name(bucket_name);
  audit_log_obj.set_remote_ip(get_header_value("X-Forwarded-For"));
  audit_log_obj.set_bytes_received(get_content_length());
  audit_log_obj.set_requester(get_account_id());
  audit_log_obj.set_request_id(request_id);
  audit_log_obj.set_operation(audit_operation_str);
  audit_log_obj.set_object_key(get_object_uri());
  audit_log_obj.set_request_uri(request_uri);
  audit_log_obj.set_http_status(http_status);
  audit_log_obj.set_signature_version(get_header_value("Authorization"));
  audit_log_obj.set_user_agent(get_header_value("User-Agent"));
  audit_log_obj.set_version_id(get_query_string_value("versionId"));
  // Setting object size for PUT object request
  if (object_size != 0) {
    audit_log_obj.set_object_size(object_size);
  }
  audit_log_obj.set_host_header(get_header_value("Host"));

  audit_log(audit_log_obj);
  s3_log(S3_LOG_DEBUG, request_id, "Exiting");
}

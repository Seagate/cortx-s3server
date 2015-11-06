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

#include <string>
#include "s3_request_object.h"
#include "s3_error_codes.h"

// evhttp Helpers
/* evhtp_kvs_iterator */
extern "C" int consume_header(evhtp_kv_t * kvobj, void * arg) {
    S3RequestObject* request = (S3RequestObject*)arg;
    request->in_headers_copy[kvobj->key] = kvobj->val ? kvobj->val : "";
    return 0;
}

S3RequestObject::S3RequestObject(evhtp_request_t *req) : ev_req(req), in_headers_copied(false) {
  printf("S3RequestObject created.\n");
  bucket_name = object_name = user_name = user_id = account_name = account_id = "";
  request_id = "TODO-Gen uuid";
}

S3RequestObject::~S3RequestObject(){
  printf("S3RequestObject deleted.\n");
}

S3HttpVerb S3RequestObject::http_verb() {
  return (S3HttpVerb)ev_req->method;
}

const char* S3RequestObject::c_get_full_path() {
  // TODO
  return ev_req->uri->path->full;
}

char * S3RequestObject::c_get_file_name() {
  return ev_req->uri->path->file;
}

unsigned char* S3RequestObject::c_get_uri_query() {
  return ev_req->uri->query_raw;
}

std::map<std::string, std::string> S3RequestObject::get_in_headers_copy(){
  if(!in_headers_copied) {
    evhtp_kvs_for_each(ev_req->headers_in, consume_header, this);
    in_headers_copied = true;
  }
  return in_headers_copy;
}

std::string S3RequestObject::get_header_value(std::string key) {
  return evhtp_header_find(ev_req->headers_in, key.c_str());
}

std::string S3RequestObject::get_host_header() {
  std::string host = "Host";
  return get_header_value(host);
}

void S3RequestObject::set_out_header_value(std::string key, std::string value) {
  evhtp_headers_add_header(ev_req->headers_out,
         evhtp_header_new(key.c_str(), value.c_str(), 1, 1));
}

size_t S3RequestObject::get_content_length() {
  std::string content_length = "Content-Length";
  return std::stoi(get_header_value(content_length));
}

std::string& S3RequestObject::get_full_body_content_as_string() {

  full_request_body = "";

  size_t num_of_extents = evbuffer_peek(buffer_in(), get_content_length() /*-1*/, NULL, NULL, 0);

  /* do the actual peek */
  struct evbuffer_iovec *vec_in = NULL;
  vec_in = (struct evbuffer_iovec *)malloc(num_of_extents * sizeof(struct evbuffer_iovec));

  /* do the actual peek at data */
  evbuffer_peek(buffer_in(), get_content_length(), NULL/*start of buffer*/, vec_in, num_of_extents);

  for (size_t i = 0; i < num_of_extents; i++) {
    full_request_body.append((char*)vec_in[i].iov_base, vec_in[i].iov_len);
  }

  free(vec_in);
  return full_request_body;
}

std::string S3RequestObject::get_query_string_value(std::string key) {
  const char *value = evhtp_kv_find(ev_req->uri->query, key.c_str());
  std::string val_str = "";
  if (value) {
    val_str = value;
  }
  return val_str;
}

bool S3RequestObject::has_query_param_key(std::string key) {
  return evhtp_kvs_find_kv(ev_req->uri->query, key.c_str()) != NULL;
}

// Operation params.
std::string S3RequestObject::get_object_uri() {
  return bucket_name + "/" + object_name;
}

void S3RequestObject::set_bucket_name(std::string& name) {
  bucket_name = name;
}

std::string& S3RequestObject::get_bucket_name() {
  return bucket_name;
}

void S3RequestObject::set_object_name(std::string& name) {
  object_name = name;
}

std::string& S3RequestObject::get_object_name() {
  return object_name;
}

void S3RequestObject::set_user_name(std::string& name) {
  user_name = name;
}

std::string& S3RequestObject::get_user_name() {
  return user_name;
}

void S3RequestObject::set_user_id(std::string& id) {
  user_id = id;
}

std::string& S3RequestObject::get_user_id() {
  return user_id;
}

void S3RequestObject::set_account_id(std::string& id) {
  account_id = id;
}

std::string& S3RequestObject::get_account_id() {
  return account_id;
}

void S3RequestObject::set_account_name(std::string& name) {
  account_name = name;
}

std::string& S3RequestObject::get_account_name() {
  return account_name;
}

std::string& S3RequestObject::get_request_id() {
  return request_id;
}

void S3RequestObject::send_response(int code, std::string body) {
  // If body not empty, write to response body. TODO
  if (!body.empty()) {
    evbuffer_add_printf(ev_req->buffer_out, body.c_str());
  }
  evhtp_send_reply(ev_req, code);
}

void S3RequestObject::send_reply_start(int code) {
  evhtp_send_reply_start(ev_req, code);
  reply_buffer = evbuffer_new();
}

void S3RequestObject::send_reply_body(char *data, int length) {
  evbuffer_add(reply_buffer, data, length);
  evhtp_send_reply_body(ev_req, reply_buffer);
}

void S3RequestObject::send_reply_end() {
  evhtp_send_reply_end(ev_req);
  evbuffer_free(reply_buffer);
}

void S3RequestObject::respond_unsupported_api() {
  S3Error error("NotImplemented", get_request_id(), "");
  std::string& response_xml = error.to_xml();
  set_out_header_value("Content-Type", "application/xml");
  set_out_header_value("Content-Length", std::to_string(response_xml.length()));

  send_response(error.get_http_status_code(), response_xml);
}


#include <string>
#include "s3_request_object.h"

S3RequestObject::S3RequestObject(evhtp_request_t *req) : ev_req(req) {
  printf("S3RequestObject created.\n");
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

std::string S3RequestObject::get_header_value(std::string& key) {
  return evhtp_header_find(ev_req->headers_in, key.c_str());
}

std::string S3RequestObject::get_host_header() {
  std::string host = "Host";
  return get_header_value(host);
}

void S3RequestObject::set_out_header_value(std::string& key, std::string& value) {
  evhtp_headers_add_header(ev_req->headers_out,
         evhtp_header_new(key.c_str(), value.c_str(), 1, 1));
}

size_t S3RequestObject::get_content_length() {
  std::string content_length = "Content-Length";
  return std::stoi(get_header_value(content_length));
}

bool S3RequestObject::has_query_param_key(std::string key) {
  return evhtp_kvs_find_kv(ev_req->uri->query, key.c_str()) != NULL;
}

// Operation params.
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
  object_name = name;
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

void S3RequestObject::send_response(int code, std::string body) {
  // If body not empty, write to response body. TODO
  if (!body.empty()) {
    evbuffer_add_printf(ev_req->buffer_out, body.c_str());
  }
  evhtp_send_reply(ev_req, code);
}

void S3RequestObject::respond_unsupported_api() {
  evbuffer_add_printf(ev_req->buffer_out, "Unsupported API endpoint.\n");
  evhtp_send_reply(ev_req, EVHTP_RES_NOTFOUND);
}

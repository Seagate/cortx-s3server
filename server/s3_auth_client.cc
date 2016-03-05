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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <unistd.h>

#include "s3_common.h"
#include "s3_auth_client.h"

/* S3 Auth client callbacks */

static void escape_char(char c, std::string& destination)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%%%.2X", (int)(unsigned char)c);
  destination.append(buf);
}

static bool char_needs_url_encoding(char c)
{
  if (c <= 0x20 || c >= 0x7f)
    return true;

  switch (c) {
    case 0x22:
    case 0x23:
    case 0x25:
    case 0x26:
    case 0x2B:
    case 0x2C:
    case 0x2F:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3E:
    case 0x3D:
    case 0x3F:
    case 0x40:
    case 0x5B:
    case 0x5D:
    case 0x5C:
    case 0x5E:
    case 0x60:
    case 0x7B:
    case 0x7D:
      return true;
  }
  return false;
}


std::string  url_encode(const char* src)
{
  std::string encoded_string = "";
  size_t len = strlen(src);
  for (size_t i = 0; i < len; i++, src++) {
    if (char_needs_url_encoding(*src)) {
      escape_char(*src, encoded_string);
    } else {
    encoded_string.append(src, 1);
    }
  }
  return encoded_string;
}

bool validate_auth_response(char *auth_response_body, S3AsyncOpContextBase *context, std::string& msg, std::string& error_code) {
  bool authsuccess = true;
  xmlNode *child_node;
  xmlChar *key = NULL;
  std::string account_details;
  std::shared_ptr<S3RequestObject> request;

  if(auth_response_body == NULL) {
    s3_log(S3_LOG_ERROR, "XML response is NULL\n");
    return false;
  }

  s3_log(S3_LOG_DEBUG, "Parsing auth xml response = %s\n", auth_response_body);
  xmlDocPtr document = xmlParseDoc((const xmlChar*)auth_response_body);
  if (document == NULL ) {
    s3_log(S3_LOG_ERROR, "Auth response XML request body Invalid.\n");
    return false;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);

  if(root_node == NULL) {
    s3_log(S3_LOG_ERROR, "Auth response XML body is Invalid.\n");
    xmlFreeDoc(document);
    return false;
  }
  xmlNodePtr child = root_node->xmlChildrenNode;
  request = context->get_request();
  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"AuthenticateUserResult"))) {
      authsuccess = true;
      for(child_node = child->children; child_node != NULL; child_node = child_node->next) {
        key = xmlNodeGetContent(child_node);
        if((!xmlStrcmp(child_node->name, (const xmlChar *)"UserId"))) {
          s3_log(S3_LOG_DEBUG, "UserId = %s\n",(char*)key);
          account_details = (char*)key;
          request->set_user_id(account_details);
        } else if((!xmlStrcmp(child_node->name, (const xmlChar *)"UserName"))) {
          s3_log(S3_LOG_DEBUG, "UserName = %s\n",(char*)key);
          account_details = (char*)key;
          request->set_user_name(account_details);
        } else if((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountName"))) {
          s3_log(S3_LOG_DEBUG, "AccountName = %s\n",(char*)key);
          account_details = (char*)key;
          request->set_account_name(account_details);
        } else if((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountId"))) {
          s3_log(S3_LOG_DEBUG, "AccountId =%s\n",(char*)key);
          account_details = (char*)key;
          request->set_account_id(account_details);
        }
        if(key != NULL) {
          xmlFree(key);
          key = NULL;
        }
      }

        //xmlFreeDoc(document);
      } else if((!xmlStrcmp(child->name, (const xmlChar *)"Code"))) {
          key = xmlNodeGetContent(child);
          error_code = (char *)key;
          if( key != NULL ) {
            xmlFree(key);
            key = NULL;
          }
          authsuccess = false;
        } else if ((!xmlStrcmp(child->name, (const xmlChar *)"Message"))) {
          key = xmlNodeGetContent(child);
          msg = (char *)key;
          if(key != NULL) {
            xmlFree(key);
            key = NULL;
          }
          authsuccess = false;
        }
       child = child->next;
     }
   if(key != NULL)
     xmlFree(key);
   xmlFreeDoc(document);
   return authsuccess;
}

extern "C" void auth_response(evhtp_request_t * req, evbuf_t * buf, void * arg) {
  std::string msg;
  std::string error_code;
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "req->status= %d\n", req->status);



  bool is_auth_successful = false;
  size_t buffer_len = evbuffer_get_length(buf) + 1;
  char *auth_response_body = (char*)malloc(buffer_len * sizeof(char));
  memset(auth_response_body, 0, buffer_len);
  evbuffer_copyout(buf, auth_response_body, buffer_len);
  s3_log(S3_LOG_DEBUG, "Data received from Auth service = %s\n\n\n", auth_response_body);

  S3AsyncOpContextBase *context = (S3AsyncOpContextBase*)arg;

  is_auth_successful = validate_auth_response(auth_response_body, context, error_code, msg);
  if (is_auth_successful) {
    s3_log(S3_LOG_DEBUG, "Authentication successful\n");
    context->set_op_status_for(0, S3AsyncOpStatus::success, "Success.");
  } else {
    s3_log(S3_LOG_ERROR, "Authentication unsuccessful\n");
    context->set_op_status_for(0, S3AsyncOpStatus::failed, msg);
  }

  if (context->get_op_status_for(0) == S3AsyncOpStatus::success) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
  free(auth_response_body);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return;
}

/******/

S3AuthClient::S3AuthClient(std::shared_ptr<S3RequestObject> req) : request(req), state(S3AuthClientOpState::init) {
  auth_client_op_context = NULL;
}

bool S3AuthClient::setup_auth_request_body(struct s3_auth_op_context *auth_ctx) {
  const char * full_path;
  const char * uri_query;
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if(auth_ctx == NULL) {
    return false;
  }
  if (auth_ctx->isfirstpass) {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out,"Action=AuthenticateUser&");
  }
  else {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out,"&Action=AuthenticateUser&");
  }
  if (request->http_verb() == S3HttpVerb::GET)
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Method=GET&");
  else if(request->http_verb() == S3HttpVerb::HEAD)
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Method=HEAD&");
  else if(request->http_verb() == S3HttpVerb::PUT)
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Method=PUT&");
  else if(request->http_verb() == S3HttpVerb::DELETE)
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Method=DELETE&");
  else if(request->http_verb() == S3HttpVerb::POST)
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Method=POST&");

  full_path = request->c_get_full_path();
  if(full_path != NULL) {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientAbsoluteUri=%s&", full_path);
  } else {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientAbsoluteUri=&");
  }
  uri_query = request->c_get_uri_query();
  if(uri_query != NULL) {
    s3_log(S3_LOG_DEBUG, "c_get_uri_query = %s\n",uri_query);
    std::string encoded_string = url_encode((char *)uri_query);
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientQueryParams=%s&", encoded_string.c_str());
  } else {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientQueryParams=&");
  }

  // May need to take it from config
  evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Version=2010-05-08");
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return true;
}

extern "C" int
setup_auth_request_headers(evhtp_header_t *header, void *arg) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  struct s3_auth_op_context *auth_ctx = (struct s3_auth_op_context *)arg;
  if(auth_ctx->isfirstpass == true) {
    std::string encoded_string = url_encode((char *)header->val);
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "%s=%s", header->key, encoded_string.c_str());
  } else {
    std::string encoded_string = url_encode((char *)header->val);
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "&%s=%s", header->key, encoded_string.c_str());
  }
  s3_log(S3_LOG_DEBUG, "key = %s and val = %s\n", header->key, header->val);

  auth_ctx->isfirstpass = false;

  if(strcmp(header->key, "Host") == 0) {
    std::string encoded_string = url_encode((char *)header->val);
    evhtp_headers_add_header(auth_ctx->authrequest->headers_out,
                              evhtp_header_new(header->key, encoded_string.c_str(), 1, 1));
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return 0;
}

extern "C" int
debug_print_auth_header(evhtp_header_t * header, void * arg) {
  s3_log(S3_LOG_DEBUG, "Header:%s Value:%s\n", header->key, header->val);
  return 0;
}

void S3AuthClient::execute_authconnect_request(struct s3_auth_op_context* auth_ctx) {
  evhtp_make_request(auth_ctx->conn, auth_ctx->authrequest, htp_method_POST, "/");
  evhtp_send_reply_body(auth_ctx->authrequest, auth_ctx->authrequest->buffer_out);
}

void S3AuthClient::check_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  evhtp_request_t *ev_req = NULL;
  size_t out_len = 0;
  char sz_size[100] = {0};


  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  ev_req = request->get_request();

  auth_context.reset(new S3AuthClientContext(request, std::bind( &S3AuthClient::check_authentication_successful, this), std::bind( &S3AuthClient::check_authentication_failed, this)));

  auth_context->init_auth_op_ctx(request->get_evbase());
  struct s3_auth_op_context* auth_ctx = auth_context->get_auth_op_ctx();
  auth_ctx->auth_callback = (evhtp_hook)auth_response;
  // Setup the auth callbacks
  evhtp_set_hook(&auth_ctx->authrequest->hooks, evhtp_hook_on_read, auth_ctx->auth_callback, auth_context.get());
  // Setup the headers to forward to auth service
  evhtp_headers_for_each(ev_req->headers_in, setup_auth_request_headers, auth_ctx);

  // Setup the body to be sent to auth service
  setup_auth_request_body(auth_ctx);

  out_len = evbuffer_get_length(auth_ctx->authrequest->buffer_out);
  sprintf(sz_size, "%zu", out_len);
  evhtp_headers_add_header(auth_ctx->authrequest->headers_out, evhtp_header_new("Content-Length", sz_size, 1, 1));
  evhtp_headers_add_header(auth_ctx->authrequest->headers_out,
    evhtp_header_new("Content-Type", "application/x-www-form-urlencoded", 0, 0));
  evhtp_headers_add_header(auth_ctx->authrequest->headers_out,
    evhtp_header_new("User-Agent", "s3server", 1, 1));
  evhtp_headers_add_header(auth_ctx->authrequest->headers_out,
    evhtp_header_new("Connection", "close", 1, 1));
  // TODO remove debug
  evhtp_headers_for_each(auth_ctx->authrequest->headers_out, debug_print_auth_header, NULL);
  char mybuff[1000] = {0};
  evbuffer_copyout(auth_ctx->authrequest->buffer_out, mybuff, sizeof(mybuff));
  s3_log(S3_LOG_DEBUG, "Data being send to Auth server:\n%s\n", mybuff);

  execute_authconnect_request(auth_ctx);

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::check_authentication_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3AuthClientOpState::authenticated;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AuthClient::check_authentication_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Authentication failure\n");
  state = S3AuthClientOpState::unauthenticated;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

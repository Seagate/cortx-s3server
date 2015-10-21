
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

static bool validate_auth_response(char *auth_response_body) {
  if(auth_response_body == NULL) {
    return false;
  }

  printf("Parsing xml request = %s\n", auth_response_body);
  xmlDocPtr document = xmlParseDoc((const xmlChar*)auth_response_body);
  if (document == NULL ) {
    printf("Auth response XML request body Invalid.\n");
    return false;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);

  if(root_node == NULL) {
    printf("Auth response XML body is Invalid.\n");
    xmlFreeDoc(document);
    return false;
  }

  xmlNodePtr child = root_node->xmlChildrenNode;
  xmlChar *key = NULL;
  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"AuthenticateUserResult"))) {
      key = xmlNodeGetContent(child);

      if (key == NULL) {
        printf("Auth XML response is Invalid.\n");
        xmlFree(key);
        xmlFreeDoc(document);
        return false;
      }
      printf("key = %s\n", (char*)key);
      if((xmlStrcmp(key,(const xmlChar *)"True") == 0))
      {
        xmlFree(key);
        xmlFreeDoc(document);
        printf("XML True found\n");
        return true;
      }
      else
      {
        xmlFree(key);
        xmlFreeDoc(document);
        printf("XML True not found \n");
        return false;
      }
    } else {
      child = child->next;
    }
 }  // While
   xmlFree(key);
   xmlFreeDoc(document);
   return false;
}

extern "C" void auth_response(evhtp_request_t * req, evbuf_t * buf, void * arg) {
  printf("Called S3AuthClient auth_response callback\n");
  printf("req->status= %d\n", req->status);

  bool is_auth_successful = false;
  size_t buffer_len = evbuffer_get_length(buf) + 1;
  char *auth_response_body = (char*)malloc(buffer_len * sizeof(char));
  memset(auth_response_body, 0, buffer_len);
  evbuffer_copyout(buf, auth_response_body, buffer_len);
  printf("Data received from Auth service = %s\n\n\n", auth_response_body);

  S3AsyncOpContextBase *context = (S3AsyncOpContextBase*)arg;

  is_auth_successful = validate_auth_response(auth_response_body);
  if (is_auth_successful) {
    printf("Authentication successful\n");
    context->set_op_status(S3AsyncOpStatus::success, "Success.");
  } else {
    printf("Authentication unsuccessful\n");
    context->set_op_status(S3AsyncOpStatus::failed, "Operation Failed.");
  }

  if (context->get_op_status() == S3AsyncOpStatus::success) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }

  return;
}

/******/

S3AuthClient::S3AuthClient(std::shared_ptr<S3RequestObject> req) : request(req), state(S3AuthClientOpState::init) {
  auth_client_op_context = NULL;
}

void S3AuthClient::setup_auth_request_body(struct s3_auth_op_context *auth_ctx) {
  printf("S3AuthClient::setup_auth_request_body\n");

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

  printf("c_get_full_path = %s\n",request->c_get_full_path());
  if(request->c_get_full_path() != NULL) {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientAbsoluteUri=%s&", request->c_get_full_path());
  } else {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientAbsoluteUri=&");
  }

  if(request->c_get_uri_query() != NULL) {
    printf("c_get_uri_query = %s\n",request->c_get_uri_query());
    std::string encoded_string = url_encode((char *)request->c_get_uri_query());
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientQueryParams=%s&", encoded_string.c_str());
  } else {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "ClientQueryParams=&");
  }

  // May need to take it from config
  evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "Version=2010-05-08");

  return;
}

extern "C" int
setup_auth_request_headers(evhtp_header_t *header, void *arg) {
  printf("Calling setup_auth_request_headers\n");

  struct s3_auth_op_context *auth_ctx = (struct s3_auth_op_context *)arg;
  if(auth_ctx->isfirstpass == true) {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "%s=%s", header->key, header->val);
  } else {
    evbuffer_add_printf(auth_ctx->authrequest->buffer_out, "&%s=%s", header->key, header->val);
  }
  printf("key = %s and val = %s\n", header->key, header->val);

  auth_ctx->isfirstpass = false;

  if(strcmp(header->key, "Host") == 0) {
    evhtp_headers_add_header(auth_ctx->authrequest->headers_out,
                              evhtp_header_new(header->key, header->val, 1, 1));
  }

  return 0;
}

extern "C" int
debug_print_auth_header(evhtp_header_t * header, void * arg) {
  printf("Header:%s Value:%s\n", header->key, header->val);
  return 0;
}


void S3AuthClient::check_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3AuthClient::check_authentication\n");
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
  printf("Data being send to Auth server:\n%s\n", mybuff);

  evhtp_make_request(auth_ctx->conn, auth_ctx->authrequest, htp_method_POST, "/");

  evhtp_send_reply_body(auth_ctx->authrequest, auth_ctx->authrequest->buffer_out);

  printf("Return from S3AuthClient::check_authentication\n");
}

void S3AuthClient::check_authentication_successful() {
  state = S3AuthClientOpState::authenticated;
  this->handler_on_success();
}

void S3AuthClient::check_authentication_failed() {
  state = S3AuthClientOpState::unauthenticated;
  this->handler_on_failed();
}

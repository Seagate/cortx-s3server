#include <event2/event.h>
#include <evhttp.h>
#include <unistd.h>
#include <string>

#include "s3_auth_client.h"
#include "s3_common.h"
#include "s3_error_codes.h"
#include "s3_post_to_main_loop.h"

void s3_auth_dummy_op_failed(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  evbuf_t *req_body_buffer = NULL;
  evhtp_request_t *req = NULL;
  std::string xml_error(
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<ErrorResponse xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<Error>"
      "<Code>InvalidAccessKeyId</Code>"
      "<Message>The AWS access key Id you provided does not exist in our "
      "records.</Message>"
      "</Error>"
      "<RequestId>0000</RequestId>"
      "</ErrorResponse>");

  struct user_event_context *user_context =
      (struct user_event_context *)user_data;
  S3AuthClientOpContext *context =
      (S3AuthClientOpContext *)user_context->app_ctx;

  req_body_buffer = evbuffer_new();
  evbuffer_add(req_body_buffer, xml_error.c_str(), xml_error.length());
  on_auth_response(req, req_body_buffer, context);
  evbuffer_free(req_body_buffer);
  // Free user event
  event_free((struct event *)user_context->user_event);
  free(user_data);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

int s3_auth_fake_evhtp_request(S3AuthClientOpType auth_request_type,
                               S3AuthClientOpContext *context) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");

  if (auth_request_type == S3AuthClientOpType::authentication) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(
        1, sizeof(struct user_event_context));
    user_ctx->app_ctx = context;
    S3PostToMainLoop((void *)user_ctx)(s3_auth_dummy_op_failed);
  } else {
    // TODO: fake cb for authorization when server support is added
    s3_log(S3_LOG_FATAL, "", "Unrecognized S3AuthClientOpType \n");
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

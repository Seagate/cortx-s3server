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
 * Author:  Abrarahmed Momin  <abrar.habib@seagate.com>
 * Original creation date: 14th-Oct-2016
 */

#include <unistd.h>
#include <string>
#include <event2/event.h>
#include <evhttp.h>

#include "s3_common.h"
#include "s3_error_codes.h"
#include "s3_auth_client.h"
#include "s3_post_to_main_loop.h"

void s3_auth_dummy_op_failed(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  evbuf_t *req_body_buffer = NULL;
  evhtp_request_t *req = NULL;
  std::string xml_error(
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<Code>InvalidAccessKeyId</Code>"
      "<Message>The AWS access key Id you provided does not exist in our "
      "records.</Message>"
      "<RequestId>0000</RequestId>"
      "</Error>");

  req_body_buffer = evbuffer_new();
  evbuffer_add(req_body_buffer, xml_error.c_str(), xml_error.length());
  on_auth_response(req, req_body_buffer, user_data);
  evbuffer_free(req_body_buffer);

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

int s3_auth_fake_evhtp_request(S3AuthClientOpType auth_request_type,
                               S3AuthClientOpContext *context) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (auth_request_type == S3AuthClientOpType::authentication) {
    S3PostToMainLoop((void *)context)(s3_auth_dummy_op_failed);
  } else {
    // TODO: fake cb for authorization when server support is added
    s3_log(S3_LOG_FATAL, "Unrecognized S3AuthClientOpType \n");
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return 0;
}

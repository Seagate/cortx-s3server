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

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_AUTH_CLIENT_H__
#define __MERO_FE_S3_SERVER_S3_AUTH_CLIENT_H__

#include <memory>
#include <functional>
#include <gtest/gtest_prod.h>

#include "s3_request_object.h"
#include "s3_auth_context.h"
#include "s3_asyncop_context_base.h"

extern "C" void auth_response(evhtp_request_t * req, evbuf_t * buf, void * arg);

class S3AuthClientContext : public S3AsyncOpContextBase {
  struct s3_auth_op_context* auth_client_op_context;
  bool has_auth_op_context;

public:
  S3AuthClientContext(std::shared_ptr<S3RequestObject> req, std::function<void()> success_callback, std::function<void()> failed_callback) : S3AsyncOpContextBase(req, success_callback, failed_callback) {
    printf("S3AuthClientContext created.\n");

    auth_client_op_context = NULL;
    has_auth_op_context = false;
  }

  ~S3AuthClientContext() {
    printf("S3AuthClientContext deleted.\n");
    if (has_auth_op_context) {
      free_basic_auth_client_op_ctx(auth_client_op_context);
      auth_client_op_context = NULL;
    }
  }

  // Call this when you want to do auth op.
  virtual bool init_auth_op_ctx(struct event_base* eventbase) {
    if(eventbase == NULL)
      return false;
    auth_client_op_context = create_basic_auth_op_ctx(eventbase);
    if(auth_client_op_context == NULL) {
      return false;
    }
    has_auth_op_context = true;
    return true;
  }

  struct s3_auth_op_context* get_auth_op_ctx() {
    return auth_client_op_context;
  }

  struct s3_auth_op_context* get_ownership_auth_client_op_ctx() {
    has_auth_op_context = false;  // release ownership, caller should free.
    return auth_client_op_context;
  }

  FRIEND_TEST(S3AuthClientContextTest, Constructor);
  FRIEND_TEST(S3AuthClientContextTest, ReleaseOwnerShipAuth);
};

enum class S3AuthClientOpState {
  init,
  started,
  authenticated,
  unauthenticated,
};

class S3AuthClient {
private:

  std::shared_ptr<S3RequestObject> request;
  std::unique_ptr<S3AuthClientContext> auth_context;

  struct s3_auth_op_context * auth_client_op_context;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3AuthClientOpState state;

  // Helper
  bool setup_auth_request_body(struct s3_auth_op_context *auth_ctx);
  virtual void execute_authconnect_request(struct s3_auth_op_context* auth_ctx);

public:
  S3AuthClient(std::shared_ptr<S3RequestObject> req);

  S3AuthClientOpState get_state() {
    return state;
  }
  virtual ~S3AuthClient() {};
  // async read
  void check_authentication(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void check_authentication_successful();
  void check_authentication_failed();

  FRIEND_TEST(S3AuthClientTest, Constructor);
  FRIEND_TEST(S3AuthClientTest, SetUpAuthRequestBody);
  FRIEND_TEST(S3AuthClientCheckTest, CheckGetAuth);
  FRIEND_TEST(S3AuthClientCheckTest, CheckPutAuth);
  FRIEND_TEST(S3AuthClientCheckTest, CheckDelAuth);
  FRIEND_TEST(S3AuthClientCheckTest, CheckHeadAuth);
  FRIEND_TEST(S3AuthClientCheckTest, CheckPostAuth);
};

#endif

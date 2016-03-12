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
 * Original author:  Rajesh Nambiar <rajesh.nambiar@seagate.com>
 * Original creation date: 22-Nov-2015
 */

#pragma once

#ifndef __MERO_FE_S3_UT_MOCK_S3_AUTH_CLIENT_H__
#define __MERO_FE_S3_UT_MOCK_S3_AUTH_CLIENT_H__

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "s3_auth_client.h"

class MockS3AuthClientOpContext : public S3AuthClientOpContext {
public:
  MockS3AuthClientOpContext(std::shared_ptr<S3RequestObject>req, std::function<void()> success_callback, std::function<void()> failed_callback) : S3AuthClientOpContext(req, success_callback, failed_callback) {}
  MOCK_METHOD0(init_auth_op_ctx, bool());
  MOCK_METHOD1(create_basic_auth_op_ctx, struct s3_auth_op_context *(struct event_base* eventbase));
};

class MockS3AuthClient : public S3AuthClient {
public:
  MockS3AuthClient(std::shared_ptr<S3RequestObject>req) : S3AuthClient(req) {}
  MOCK_METHOD1(execute_authconnect_request, void(struct s3_auth_op_context *auth_ctx));
};
#endif

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
 * Original creation date: 19-Nov-2015
 */

#include "gtest/gtest.h"
#include <functional>
#include "s3_auth_client.h"
#include <iostream>
#include <memory>
#include "mock_s3_asyncop_context_base.h"
#include "mock_s3_request_object.h"
#include "mock_s3_auth_client.h"
#include "mock_evhtp_wrapper.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::Return;
using ::testing::Mock;
using ::testing::InvokeWithoutArgs;

static void
dummy_request_cb(evhtp_request_t * req, void * arg) {
}

void dummy_function() {
  return;
}

class S3AuthBaseResponse {
  public:
    virtual void auth_response_wrapper(evhtp_request_t * req, evbuf_t * buf, void * arg) = 0;
};

class S3AuthResponse : public S3AuthBaseResponse {
  public:
    S3AuthResponse() {
      success_called = fail_called = false;
    }
    virtual void auth_response_wrapper(evhtp_request_t * req, evbuf_t * buf, void * arg) {
      return auth_response(req, buf, arg);
    }

    void success_callback() {
      success_called = true;
    }

    void fail_callback() {
      fail_called = true;
    }

   bool success_called;
   bool fail_called;
};

class S3AuthClientContextTest : public testing::Test {
  protected:
    S3AuthClientContextTest() {
      evhtp_request_t * req = NULL;
      EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
      ptr_mock_request = std::make_shared<MockS3RequestObject> (req, evhtp_obj_ptr);
      success_callback = NULL;
      failed_callback = NULL;
      p_authtest = new S3AuthClientContext(ptr_mock_request, success_callback, failed_callback);
    }

  ~S3AuthClientContextTest() {
    delete p_authtest;
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::function<void()> success_callback;
  std::function<void()> failed_callback;
  S3AuthClientContext *p_authtest;
};

class S3AuthClientTest : public testing::Test {
  protected:
    S3AuthClientTest() {
      evhtp_request_t * req = NULL;
      EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
      ptr_mock_request = std::make_shared<MockS3RequestObject> (req, evhtp_obj_ptr);
      p_authclienttest = new S3AuthClient(ptr_mock_request);
      auth_client_op_context = (struct s3_auth_op_context *)calloc(1, sizeof(struct s3_auth_op_context));
      auth_client_op_context->evbase = event_base_new();
      auth_client_op_context->authrequest = evhtp_request_new(dummy_request_cb, auth_client_op_context->evbase);
    }

    ~S3AuthClientTest() {
       event_base_free(auth_client_op_context->evbase);
       free(auth_client_op_context);
       delete p_authclienttest;
    }

    S3AuthClient *p_authclienttest;
    std::shared_ptr<MockS3RequestObject> ptr_mock_request;
    struct s3_auth_op_context *auth_client_op_context;
};

class S3AuthClientCheckTest : public testing::Test {
  protected:
    S3AuthClientCheckTest() {
      evbase = event_base_new();
      req = evhtp_request_new(dummy_request_cb, evbase);
      EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
      ptr_mock_request = std::make_shared<MockS3RequestObject> (req, evhtp_obj_ptr);
      ptr_mock_auth_context = std::make_shared<MockS3AuthClientContext> (ptr_mock_request, std::bind(dummy_function), std::bind(dummy_function));
      ptr_auth_client = std::make_shared<MockS3AuthClient>(ptr_mock_request);
    }

    void fake_in_headers(std::map<std::string, std::string> hdrs_in) {
      for(auto itr : hdrs_in) {
        evhtp_headers_add_header(req->headers_in,
                                 evhtp_header_new(itr.first.c_str(), itr.second.c_str(), 0, 0));
      }
    }

    void fake_auth_in_headers() {
      input_headers["Content-Length"] = "0";
      input_headers["Host"] = "seagate_bucket.s3.seagate.com";
      input_headers["Authorization"] = "AWS AKIAJTYX36YCKQSAJT7Q:Cv1jfcl/5MRXJOHGKyosnoeNIk8=";
      input_headers["x-amz-date"] = "Tue, 24 Nov 2015 04:18:26 +0000";
      fake_in_headers(input_headers);
    }

    ~S3AuthClientCheckTest() {
       event_base_free(evbase);
     }

    evbase_t *evbase;
    evhtp_request_t * req;
    struct s3_auth_op_context * auth_client_op_context;
    std::map<std::string, std::string> input_headers;
    std::shared_ptr<MockS3AuthClient> ptr_auth_client;
    std::shared_ptr<MockS3RequestObject> ptr_mock_request;
    std::shared_ptr<MockS3AuthClientContext> ptr_mock_auth_context;
    S3AuthResponse s3authrespobj;
};

class S3AuthResponseTest : public testing::Test {
  protected:
    S3AuthResponseTest() {
      evbase = event_base_new();
      ev_request = evhtp_request_new(dummy_request_cb, evbase);
      EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
      ptr_mock_request = std::make_shared<MockS3RequestObject> (ev_request, evhtp_obj_ptr);
      ptr_mock_async = new MockS3AsyncOpContextBase(ptr_mock_request, std::bind(dummy_function), std::bind(dummy_function));
  }
  evhtp_request_t *ev_request;
  evbase_t *evbase;
  S3AuthResponse  s3authrespobj;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  MockS3AsyncOpContextBase *ptr_mock_async;
  ~S3AuthResponseTest() {
     event_base_free(evbase);
     free(ptr_mock_async);
  }
};

TEST_F(S3AuthClientContextTest, Constructor) {
  EXPECT_EQ(NULL, p_authtest->auth_client_op_context);
  EXPECT_EQ(false, p_authtest->has_auth_op_context);
}

TEST_F(S3AuthClientContextTest, InitAuthCtxNull) {
  bool ret;
  ret = p_authtest->init_auth_op_ctx(NULL);
  EXPECT_EQ(false, ret);
}

TEST_F(S3AuthClientContextTest, InitAuthCtxValid) {
  bool ret;
  event_base* eventbase= event_base_new();
  ret = p_authtest->init_auth_op_ctx(eventbase);
  event_base_free(eventbase);
  EXPECT_TRUE(ret == true);
}

TEST_F(S3AuthClientContextTest, GetAuthCtx) {
  struct s3_auth_op_context *p_ctx;
  event_base* eventbase= event_base_new();
  p_authtest->init_auth_op_ctx(eventbase);
  p_ctx = p_authtest->get_auth_op_ctx();
  event_base_free(eventbase);
  EXPECT_TRUE(p_ctx != NULL);
}

TEST_F(S3AuthClientContextTest, ReleaseOwnerShipAuth) {
  struct s3_auth_op_context *p_ctx, *p_rel_ctx;
  event_base* eventbase= event_base_new();
  p_authtest->init_auth_op_ctx(eventbase);
  p_ctx = p_authtest->get_auth_op_ctx();
  event_base_free(eventbase);
  p_rel_ctx = p_authtest->get_ownership_auth_client_op_ctx();
  EXPECT_EQ(p_ctx, p_rel_ctx);
  EXPECT_TRUE(p_rel_ctx != NULL);
  EXPECT_FALSE(p_authtest->has_auth_op_context);
}

TEST_F(S3AuthClientTest, Constructor) {
  EXPECT_TRUE(p_authclienttest->auth_client_op_context == NULL);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBody) {
  bool ret;
  int len;
  char *mybuff;
  char expectedbody[] = "&Action=AuthenticateUser&Method=GET&ClientAbsoluteUri=/&ClientQueryParams=&Version=2010-05-08";
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillRepeatedly(Return("/"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillRepeatedly(Return(""));

  ret = p_authclienttest->setup_auth_request_body(NULL);
  EXPECT_TRUE(ret == false);

  ret = p_authclienttest->setup_auth_request_body(auth_client_op_context);
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_STREQ(expectedbody, mybuff);
  free(mybuff);
}

TEST_F(S3AuthClientCheckTest, CheckGetAuth) {
  int   len;
  char *mybuff;
  char expectedbody[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=GET&ClientAbsoluteUri=/&ClientQueryParams=&Version=2010-05-08";
  char body[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=GET&ClientAbsoluteUri=/seagate_bucket/3kfile&ClientQueryParams=location%3DUS%26policy%3Dtest&Version=2010-05-08";
  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .Times(2)
              .WillRepeatedly(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .Times(2)
              .WillRepeatedly(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .Times(2)
              .WillRepeatedly(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/"))
              .WillOnce(Return("/seagate_bucket/3kfile"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""))
              .WillOnce(Return("location=US&policy=test"));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_))
              .Times(2);

  fake_auth_in_headers();
  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(expectedbody, mybuff);
  free(mybuff);
  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(body, mybuff);
  free(mybuff);
}

TEST_F(S3AuthClientCheckTest, CheckPutAuth) {
  char *mybuff;
  int len;
  char expectedbody[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=PUT&ClientAbsoluteUri=/&ClientQueryParams=&Version=2010-05-08";
  char body[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=PUT&ClientAbsoluteUri=/seagate_bucket/3kfile&ClientQueryParams=location%3DUS%26policy%3Dtest&Version=2010-05-08";
  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .Times(2)
              .WillRepeatedly(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .Times(2)
              .WillRepeatedly(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillRepeatedly(Return(S3HttpVerb::PUT));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/"))
              .WillOnce(Return("/seagate_bucket/3kfile"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""))
              .WillOnce(Return("location=US&policy=test"));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_))
              .Times(2);

  fake_auth_in_headers();
  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);

  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(body, mybuff);
  free(mybuff);
}

TEST_F(S3AuthClientCheckTest, CheckDelAuth) {
  char *mybuff;
  int len;
  char expectedbody[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=DELETE&ClientAbsoluteUri=/seagate_bucket&ClientQueryParams=&Version=2010-05-08";
  char body[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=DELETE&ClientAbsoluteUri=/seagate_bucket/3kfile&ClientQueryParams=location%3DUS%26policy%3Dtest&Version=2010-05-08";

  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .Times(2)
              .WillRepeatedly(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .Times(2)
              .WillRepeatedly(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillRepeatedly(Return(S3HttpVerb::DELETE));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/seagate_bucket"))
              .WillOnce(Return("/seagate_bucket/3kfile"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""))
              .WillOnce(Return("location=US&policy=test"));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_))
              .Times(2);
  fake_auth_in_headers();
  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);

  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(body, mybuff);
  free(mybuff);
}

TEST_F(S3AuthClientCheckTest, CheckHeadAuth) {
  char *mybuff;
  int len;
  char expectedbody[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=HEAD&ClientAbsoluteUri=/seagate_bucket&ClientQueryParams=&Version=2010-05-08";
  char body[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=HEAD&ClientAbsoluteUri=/seagate_bucket/3kfile&ClientQueryParams=location%3DUS%26policy%3Dtest&Version=2010-05-08";

  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .Times(2)
              .WillRepeatedly(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .Times(2)
              .WillRepeatedly(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillRepeatedly(Return(S3HttpVerb::HEAD));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/seagate_bucket"))
              .WillOnce(Return("/seagate_bucket/3kfile"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""))
              .WillOnce(Return("location=US&policy=test"));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_))
              .Times(2);
  fake_auth_in_headers();
  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);

  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(body, mybuff);
  free(mybuff);
}

TEST_F(S3AuthClientCheckTest, CheckPostAuth) {
  char *mybuff;
  int len;
  char expectedbody[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=POST&ClientAbsoluteUri=/seagate_bucket&ClientQueryParams=&Version=2010-05-08";
  char body[] = "Authorization=AWS%20AKIAJTYX36YCKQSAJT7Q%3ACv1jfcl%2F5MRXJOHGKyosnoeNIk8%3D&Content-Length=0&Host=seagate_bucket.s3.seagate.com&x-amz-date=Tue%2C%2024%20Nov%202015%2004%3A18%3A26%20%2B0000&Action=AuthenticateUser&Method=POST&ClientAbsoluteUri=/seagate_bucket/3kfile&ClientQueryParams=location%3DUS%26policy%3Dtest&Version=2010-05-08";
  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .Times(2)
              .WillRepeatedly(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .Times(2)
              .WillRepeatedly(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillRepeatedly(Return(S3HttpVerb::POST));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/seagate_bucket"))
              .WillOnce(Return("/seagate_bucket/3kfile"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""))
              .WillOnce(Return("location=US&policy=test"));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_))
              .Times(2);
  fake_auth_in_headers();
  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);

  ptr_auth_client->check_authentication(std::bind(dummy_function), std::bind(dummy_function));
  auth_client_op_context = ptr_auth_client->auth_context->get_auth_op_ctx();
  len = evbuffer_get_length(auth_client_op_context->authrequest->buffer_out);
  mybuff = (char *)calloc(1,len + 1);
  evbuffer_copyout(auth_client_op_context->authrequest->buffer_out,mybuff, len);
  EXPECT_FALSE(ptr_auth_client->handler_on_success == NULL);
  EXPECT_FALSE(ptr_auth_client->handler_on_failed == NULL);
  EXPECT_STREQ(body, mybuff);
  free(mybuff);
}

TEST_F(S3AuthClientCheckTest, AuthSuccessCheck) {
  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .WillOnce(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .WillOnce(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/seagate_bucket"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_));

  ptr_auth_client->check_authentication(std::bind(&S3AuthResponse::success_callback, &s3authrespobj),
                                        std::bind(&S3AuthResponse::fail_callback, &s3authrespobj));

  ptr_auth_client->check_authentication_successful();
  EXPECT_TRUE(s3authrespobj.success_called == true);
  EXPECT_TRUE(S3AuthClientOpState::authenticated == ptr_auth_client->get_state());
}

TEST_F(S3AuthClientCheckTest, AuthFailCheck) {
  EXPECT_CALL(*ptr_mock_request, get_evbase())
              .WillOnce(Return(evbase));
  EXPECT_CALL(*ptr_mock_request, get_request())
              .WillOnce(Return(req));
  EXPECT_CALL(*ptr_mock_request, http_verb())
              .WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
              .WillOnce(Return("/seagate_bucket"));
  EXPECT_CALL(*ptr_mock_request, c_get_uri_query())
              .WillOnce(Return(""));
  EXPECT_CALL(*ptr_auth_client, execute_authconnect_request(_));

  ptr_auth_client->check_authentication(std::bind(&S3AuthResponse::success_callback, &s3authrespobj),
                                        std::bind(&S3AuthResponse::fail_callback, &s3authrespobj));

  ptr_auth_client->check_authentication_failed();
  EXPECT_TRUE(s3authrespobj.fail_called == true);
  EXPECT_TRUE(S3AuthClientOpState::unauthenticated == ptr_auth_client->get_state());
}
TEST_F(S3AuthResponseTest, AuthTrueResp) {
  char authresp[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><AuthenticateUserResponse xmlns=\"https://iam.seagate.com/doc/2010-05-08/\"><AuthenticateUserResult><Authenticated>True</Authenticated></AuthenticateUserResult><ResponseMetadata><RequestId>0000</RequestId></ResponseMetadata></AuthenticateUserResponse>";
  evbuffer_add(ev_request->buffer_in,authresp, sizeof(authresp));
  s3authrespobj.auth_response_wrapper(ev_request, ev_request->buffer_in, (void*)(ptr_mock_async));
  EXPECT_TRUE(ptr_mock_async->get_op_status_for(0) == S3AsyncOpStatus::success);
}

TEST_F(S3AuthResponseTest, AuthFailResp) {
  char authresp[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><AuthenticateUserResponse xmlns=\"https://iam.seagate.com/doc/2010-05-08/\"><Code>403</Code><Message>Access Denied</Message></AuthenticateUserResponse>";
  evbuffer_add(ev_request->buffer_in,authresp, sizeof(authresp));
  s3authrespobj.auth_response_wrapper(ev_request, ev_request->buffer_in, (void*)(ptr_mock_async));
  EXPECT_TRUE(ptr_mock_async->get_op_status_for(0) == S3AsyncOpStatus::failed);
}

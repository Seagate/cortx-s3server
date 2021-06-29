/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include <memory>

#include <gtest/gtest.h>

#include "s3_auth_client.h"
#include "s3_option.h"

#include "mock_evhtp_wrapper.h"
#include "mock_s3_asyncop_context_base.h"
#include "mock_s3_auth_client.h"
#include "mock_s3_request_object.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::Return;
using ::testing::Mock;
using ::testing::InvokeWithoutArgs;
using ::testing::ReturnRef;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

void dummy_function() { return; }

class S3AuthBaseResponse {
 public:
  virtual void auth_response_wrapper(evhtp_request_t *req, evbuf_t *buf,
                                     void *arg) = 0;
};

// S3AuthResponse class

class S3AuthResponse : public S3AuthBaseResponse {
 public:
  S3AuthResponse() { success_called = fail_called = false; }
  virtual void auth_response_wrapper(evhtp_request_t *req, evbuf_t *buf,
                                     void *arg) {
    on_read_response(req, buf, arg);
  }

  void success_callback() { success_called = true; }

  void fail_callback() { fail_called = true; }

  bool success_called;
  bool fail_called;
};

static void fn_stub() {}

class S3AuthClientOpContextTest : public testing::Test {

 public:
  ~S3AuthClientOpContextTest();

 protected:
  S3AuthClientOpContextTest();
  void SetUp() override;

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::unique_ptr<S3AuthClientOpContext> p_authopctx;

 private:
  evbase_t *p_evbase, *p_evbase_old;
};

S3AuthClientOpContextTest::S3AuthClientOpContextTest() {

  p_evbase = event_base_new();

  ptr_mock_request = std::make_shared<MockS3RequestObject>(
      evhtp_request_new(NULL, p_evbase), new EvhtpWrapper());

  p_evbase_old = S3Option::get_instance()->get_eventbase();
  S3Option::get_instance()->set_eventbase(p_evbase);
}

S3AuthClientOpContextTest::~S3AuthClientOpContextTest() {
  S3Option::get_instance()->set_eventbase(p_evbase_old);
  event_base_free(p_evbase);
}

void S3AuthClientOpContextTest::SetUp() {
  p_authopctx.reset(new S3AuthClientOpContext(
      ptr_mock_request, fn_stub, fn_stub, S3AuthClientOpType::authentication));
}

// S3AuthClientTest class

class S3AuthClientTest : public testing::Test {
 protected:
  S3AuthClientTest() {
    evbase_t *p_evbase = event_base_new();
    ev_req = evhtp_request_new(NULL, p_evbase);
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(ev_req, new EvhtpWrapper());

    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy())
        .WillRepeatedly(ReturnRef(input_headers));

    ptr_mock_request->set_user_name("tester");
    ptr_mock_request->set_user_id("123");
    ptr_mock_request->set_email("abc@dummy.com");
    ptr_mock_request->set_canonical_id("123456789dummyCANONICALID");
    ptr_mock_request->set_account_name("s3_test");
    ptr_mock_request->set_account_id("12345");

    p_authclienttest = new S3AuthClient(ptr_mock_request);
    p_authclienttest->op_type = S3AuthClientOpType::authentication;

    auth_client_op_context = (struct s3_auth_op_context *)calloc(
        1, sizeof(struct s3_auth_op_context));
    auth_client_op_context->evbase = event_base_new();
    auth_client_op_context->auth_request =
        evhtp_request_new(dummy_request_cb, auth_client_op_context->evbase);
  }

  ~S3AuthClientTest() {
    event_base_free(auth_client_op_context->evbase);
    free(auth_client_op_context);
    delete p_authclienttest;
  }

  void fake_in_header(std::string key, std::string val) {
    evhtp_headers_add_header(ev_req->headers_in,
                             evhtp_header_new(key.c_str(), val.c_str(), 0, 0));
  }

  evhtp_request_t *ev_req;
  S3AuthClient *p_authclienttest;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  struct s3_auth_op_context *auth_client_op_context;

  std::map<std::string, std::string> input_headers;
};

TEST_F(S3AuthClientOpContextTest, Constructor) {
  EXPECT_EQ(NULL, p_authopctx->auth_op_context);
}

TEST_F(S3AuthClientOpContextTest, InitAuthCtxValid) {
  bool ret = p_authopctx->init_auth_op_ctx();
  EXPECT_TRUE(ret == true);
}

TEST_F(S3AuthClientOpContextTest, GetAuthCtx) {

  p_authopctx->init_auth_op_ctx();
  struct s3_auth_op_context *p_ctx = p_authopctx->get_auth_op_ctx();
  EXPECT_TRUE(p_ctx != NULL);
}

TEST_F(S3AuthClientOpContextTest, CanParseAuthSuccessResponse) {
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><AuthenticateUserResult><UserId>123</UserId><UserName>tester</"
      "UserName><AccountId>12345</AccountId><AccountName>s3_test</"
      "AccountName><SignatureSHA256>BSewvoSw/0og+hWR4I77NcWea24=</"
      "SignatureSHA256></"
      "AuthenticateUserResult><ResponseMetadata><RequestId>0000</RequestId></"
      "ResponseMetadata></AuthenticateUserResponse>";

  ptr_mock_request->set_user_id("");

  p_authopctx->handle_response(sample_response.c_str(), 200);

  EXPECT_TRUE(p_authopctx->is_success());
  EXPECT_STREQ("tester", p_authopctx->get_request()->get_user_name().c_str());
  EXPECT_STREQ("123", p_authopctx->get_request()->get_user_id().c_str());
  EXPECT_STREQ("s3_test",
               p_authopctx->get_request()->get_account_name().c_str());
  EXPECT_STREQ("12345", p_authopctx->get_request()->get_account_id().c_str());
}

TEST_F(S3AuthClientOpContextTest, CanParseAuthorizationSuccessResponse) {
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><AuthorizeUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><AuthorizeUserResult><UserId>123</UserId><UserName>tester</"
      "UserName><AccountId>12345</AccountId><AccountName>s3_test</"
      "AccountName><CanonicalId>507e9f946afa4a18b0ac54e869c5fc6b6eb518c22e3a90<"
      "/CanonicalId></AuthorizeUserResult><ResponseMetadata><RequestId>0000</"
      "RequestId></ResponseMetadata></AuthorizeUserResponse>";

  ptr_mock_request->set_user_id("");

  p_authopctx->handle_response(sample_response.c_str(), 200);

  EXPECT_TRUE(p_authopctx->is_success());
  EXPECT_STREQ("tester", p_authopctx->get_request()->get_user_name().c_str());
  EXPECT_STREQ("507e9f946afa4a18b0ac54e869c5fc6b6eb518c22e3a90",
               p_authopctx->get_request()->get_canonical_id().c_str());
  EXPECT_STREQ("123", p_authopctx->get_request()->get_user_id().c_str());
  EXPECT_STREQ("s3_test",
               p_authopctx->get_request()->get_account_name().c_str());
  EXPECT_STREQ("12345", p_authopctx->get_request()->get_account_id().c_str());
}

TEST_F(S3AuthClientOpContextTest,
       CanHandleParseErrorInAuthorizeSuccessResponse) {
  // Missing AccountId
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><AuthorizeUserResult><UserId>123</UserId><UserName>tester</"
      "UserName><AccountName>s3_test</AccountName><SignatureSHA256>BSewvoSw/"
      "0og+hWR4I77NcWea24=</SignatureSHA256></"
      "AuthenticateUserResult><ResponseMetadata><RequestId>0000</RequestId></"
      "ResponseMetadata></AuthorizeUserResponse>";

  p_authopctx->handle_response(sample_response.c_str(), 200);

  EXPECT_FALSE(p_authopctx->is_success());
}

TEST_F(S3AuthClientOpContextTest, CanHandleParseErrorInAuthSuccessResponse) {
  // Missing AccountId
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><AuthenticateUserResult><UserId>123</UserId><UserName>tester</"
      "UserName><AccountName>s3_test</AccountName><SignatureSHA256>BSewvoSw/"
      "0og+hWR4I77NcWea24=</SignatureSHA256></"
      "AuthenticateUserResult><ResponseMetadata><RequestId>0000</RequestId></"
      "ResponseMetadata></AuthenticateUserResponse>";

  p_authopctx->handle_response(sample_response.c_str(), 200);

  EXPECT_FALSE(p_authopctx->is_success());
}

TEST_F(S3AuthClientOpContextTest, CanParseAuthErrorResponse) {
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><Error><Code>SignatureDoesNotMatch</Code><Message>The request "
      "signature we "
      "calculated does not match the signature you provided. Check your AWS "
      "secret access key and signing method. For more information, see REST "
      "Authentication andSOAP Authentication for "
      "details.</Message></Error><RequestId>0000</RequestId></ErrorResponse>";

  p_authopctx->handle_response(sample_response.c_str(), 403);

  EXPECT_FALSE(p_authopctx->is_success());
  EXPECT_STREQ("SignatureDoesNotMatch", p_authopctx->get_error_code().c_str());
  EXPECT_STREQ(
      "The request signature we calculated does not match the signature you "
      "provided. Check your AWS secret access key and signing method. For more "
      "information, see REST Authentication andSOAP Authentication for "
      "details.",
      p_authopctx->get_error_message().c_str());
}

TEST_F(S3AuthClientOpContextTest, CanParseAuthInvalidTokenErrorResponse) {
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><Error><Code>InvalidToken</Code><Message>The provided "
      "token is malformed or otherwise "
      "invalid.</Message></Error><RequestId>0000</RequestId></ErrorResponse>";

  p_authopctx->handle_response(sample_response.c_str(), 403);

  EXPECT_FALSE(p_authopctx->is_success());
  EXPECT_STREQ("InvalidToken", p_authopctx->get_error_code().c_str());
  EXPECT_STREQ("The provided token is malformed or otherwise invalid.",
               p_authopctx->get_error_message().c_str());
}

TEST_F(S3AuthClientOpContextTest, CanParseAuthorizationErrorResponse) {
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><Error><Code>UnauthorizedOperation</Code><Message>You are not "
      "authorized to "
      "perform this operation. Check your IAM policies, and ensure that you "
      "are using the correct access "
      "keys.</Message></Error><RequestId>0000</RequestId></ErrorResponse>";

  p_authopctx->handle_response(sample_response.c_str(), 403);

  EXPECT_FALSE(p_authopctx->is_success());
  EXPECT_STREQ("UnauthorizedOperation", p_authopctx->get_error_code().c_str());
  EXPECT_STREQ(
      "You are not authorized to perform this operation. Check your IAM "
      "policies, and ensure that you are using the correct access keys.",
      p_authopctx->get_error_message().c_str());
}

TEST_F(S3AuthClientOpContextTest, CanHandleParseErrorInAuthErrorResponse) {
  // Missing code
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><Error "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\"><Message>The request "
      "signature we calculated does not match the signature you provided. "
      "Check your AWS secret access key and signing method. For more "
      "information, see REST Authentication andSOAP Authentication for "
      "details.</Message><RequestId>0000</RequestId></Error>";

  p_authopctx->handle_response(sample_response.c_str(), 403);

  EXPECT_FALSE(p_authopctx->error_resp_is_OK());
}

TEST_F(S3AuthClientOpContextTest, CanHandleParseErrorInAuthorizeErrorResponse) {
  // Missing code
  std::string sample_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><Error "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\"><Message>You are not "
      "authorized to perform this operation. Check your IAM policies, and "
      "ensure that you are using the correct access "
      "keys.</Message><RequestId>0000</RequestId></Error>";

  p_authopctx->handle_response(sample_response.c_str(), 403);

  EXPECT_FALSE(p_authopctx->error_resp_is_OK());
}

TEST_F(S3AuthClientTest, Constructor) {
  EXPECT_TRUE(p_authclienttest->get_state() == S3AuthClientOpState::init);
  EXPECT_FALSE(p_authclienttest->is_chunked_auth);
  EXPECT_FALSE(p_authclienttest->last_chunk_added);
  EXPECT_FALSE(p_authclienttest->chunk_auth_aborted);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyGet) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams=&Method="
      "GET&Request_id=123&RequestorAccountId=12345&RequestorAccountName=s3_"
      "test&RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));
  p_authclienttest->request_id = "123";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyPut) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams=&Method="
      "PUT&Request_id=123&RequestorAccountId=12345&RequestorAccountName=s3_"
      "test&RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::PUT));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));
  p_authclienttest->request_id = "123";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyHead) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams=&Method="
      "HEAD&Request_id=123&RequestorAccountId=12345&RequestorAccountName=s3_"
      "test&RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::HEAD));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));
  p_authclienttest->request_id = "123";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyDelete) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams=&Method="
      "DELETE&Request_id=123&RequestorAccountId=12345&RequestorAccountName=s3_"
      "test&RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::DELETE));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));
  p_authclienttest->request_id = "123";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyPost) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams=&Method="
      "POST&Request_id=123&RequestorAccountId=12345&RequestorAccountName=s3_"
      "test&RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::POST));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));
  p_authclienttest->request_id = "123";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyWithQueryParams) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams="
      "delimiter%3D%252F%26prefix%3Dtest&Method=GET&Request_id=123&"
      "RequestorAccountId=12345&RequestorAccountName=s3_test&"
      "RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;
  query_params["delimiter"] = "/";
  query_params["prefix"] = "test";

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));
  p_authclienttest->request_id = "123";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyForChunkedAuth) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams="
      "delimiter%3D%252F%26prefix%3Dtest&Method=GET&Request_id=123&"
      "RequestorAccountId=12345&RequestorAccountName=s3_test&"
      "RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;
  query_params["delimiter"] = "/";
  query_params["prefix"] = "test";

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));

  p_authclienttest->is_chunked_auth = true;
  p_authclienttest->request_id = "123";
  p_authclienttest->prev_chunk_signature_from_auth = "";
  p_authclienttest->current_chunk_signature_from_auth = "ABCD";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyForChunkedAuth1) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams="
      "delimiter%3D%252F%26prefix%3Dtest&Method=GET&Request_id=123&"
      "RequestorAccountId=12345&RequestorAccountName=s3_test&"
      "RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08";
  std::map<std::string, std::string, compare> query_params;
  query_params["delimiter"] = "/";
  query_params["prefix"] = "test";

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));

  p_authclienttest->is_chunked_auth = true;
  p_authclienttest->request_id = "123";
  p_authclienttest->prev_chunk_signature_from_auth = "ABCD";
  p_authclienttest->current_chunk_signature_from_auth = "";
  p_authclienttest->setup_auth_request_body();
  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

TEST_F(S3AuthClientTest, SetUpAuthRequestBodyForChunkedAuth2) {
  char expectedbody[] =
      "Action=AuthenticateUser&ClientAbsoluteUri=%2F&ClientQueryParams="
      "delimiter%3D%252F%26prefix%3Dtest&Method=GET&Request_id=123&"
      "RequestorAccountId=12345&RequestorAccountName=s3_test&"
      "RequestorCanonicalId=123456789dummyCANONICALID&RequestorEmail=abc%"
      "40dummy.com&RequestorUserId=123&RequestorUserName=tester&Version=2010-"
      "05-08&current-signature-sha256=cur-XYZ&previous-signature-sha256=prev-"
      "ABCD&x-amz-content-sha256=sha256-abcd";
  std::map<std::string, std::string, compare> query_params;
  query_params["prefix"] = "test";
  query_params["delimiter"] = "/";

  EXPECT_CALL(*ptr_mock_request, get_query_parameters())
      .WillRepeatedly(ReturnRef(query_params));
  EXPECT_CALL(*ptr_mock_request, http_verb())
      .WillRepeatedly(Return(S3HttpVerb::GET));
  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillRepeatedly(Return("/"));

  p_authclienttest->is_chunked_auth = true;
  p_authclienttest->request_id = "123";
  p_authclienttest->prev_chunk_signature_from_auth = "prev-ABCD";
  p_authclienttest->current_chunk_signature_from_auth = "cur-XYZ";
  p_authclienttest->hash_sha256_current_chunk = "sha256-abcd";
  p_authclienttest->setup_auth_request_body();

  int len = evbuffer_get_length(p_authclienttest->req_body_buffer);
  char *mybuff = (char *)calloc(1, len + 1);
  evbuffer_copyout(p_authclienttest->req_body_buffer, mybuff, len);

  EXPECT_STREQ(expectedbody, mybuff);

  free(mybuff);
}

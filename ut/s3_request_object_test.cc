#include "gtest/gtest.h"

#include "s3_request_object.h"
#include "s3_error_codes.h"

static void
dummy_request_cb(evhtp_request_t * req, void * arg) {
}

// To use a test fixture, derive a class from testing::Test.
class S3RequestObjectTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3RequestObjectTest() {
    // placeholder evbase
    evbase = event_base_new();
    ev_request = evhtp_request_new(dummy_request_cb, evbase);
    request = new S3RequestObject(ev_request);
  }

  ~S3RequestObjectTest() {
    delete request;
    event_base_free(evbase);
  }

  // A helper functions
  void init_uri_() {
    ev_request->uri = (evhtp_uri_t*)calloc(sizeof(evhtp_uri_t), 1);
    ev_request->uri->query = evhtp_query_new();
    ev_request->uri->path = (evhtp_path_t*)calloc(sizeof(evhtp_path_t), 1);
  }

  char* malloc_cp_c_str_(std::string str) {
    char* c_str = (char*)malloc(str.length() + 1);
    memcpy(c_str, str.c_str(), str.length());
    c_str[str.length()] = '\0';
    return c_str;
  }

  // Test setup helpers
  void fake_in_headers(std::map<std::string, std::string> hdrs_in) {
    for(auto itr : hdrs_in) {
      evhtp_headers_add_header(ev_request->headers_in,
                               evhtp_header_new(itr.first.c_str(), itr.second.c_str(), 0, 0));
    }
  }

  // For simplicity of test we take separate args and not repeat _evhtp_path_new()
  void fake_uri_path(std::string full, std::string path, std::string file) {
    init_uri_();
    ev_request->uri->path->full = malloc_cp_c_str_(full);
    ASSERT_TRUE(ev_request->uri->path->full != NULL);
    ev_request->uri->path->path = malloc_cp_c_str_(path);
    ASSERT_TRUE(ev_request->uri->path->path != NULL);
    ev_request->uri->path->file = malloc_cp_c_str_(file);
    ASSERT_TRUE(ev_request->uri->path->full != NULL);
  }

  void fake_query_params(std::string raw_query) {
    init_uri_();
    ev_request->uri->query_raw = (unsigned char*)malloc_cp_c_str_(raw_query);
    ASSERT_TRUE(ev_request->uri->query_raw != NULL);

    ev_request->uri->query = evhtp_parse_query_wflags(raw_query.c_str(), raw_query.length(), EVHTP_PARSE_QUERY_FLAG_ALLOW_NULL_VALS);
    if (!ev_request->uri->query) {
      FAIL() << "Test case setup failed due to invalid query.";
    }
  }

  void fake_buffer_in(std::string content) {
    evbuffer_add(ev_request->buffer_in, content.c_str(), content.length());
  }

  // Declares the variables your tests want to use.
  S3RequestObject *request;
  evhtp_request_t *ev_request;  // To fill test data
  evbase_t *evbase;
};

TEST_F(S3RequestObjectTest, ReturnsValidEVRequest) {
  EXPECT_EQ(ev_request, request->get_request());
}

TEST_F(S3RequestObjectTest, ReturnsValidRawQuery) {
  fake_query_params("location=US&policy=test");

  EXPECT_STREQ("location=US&policy=test", (char*)request->c_get_uri_query());
}

TEST_F(S3RequestObjectTest, ReturnsValidUriPaths) {
  fake_uri_path("/bucket_name/object_name", "/bucket_name/", "object_name");

  EXPECT_STREQ("/bucket_name/object_name", request->c_get_full_path());
  EXPECT_STREQ("object_name", request->c_get_file_name());
}

TEST_F(S3RequestObjectTest, ReturnsValidHeadersCopy) {
  std::map<std::string, std::string> input_headers;
  input_headers["Content-Length"] = "512";
  input_headers["Host"] = "kaustubh.s3.seagate.com";

  fake_in_headers(input_headers);

  EXPECT_TRUE(input_headers == request->get_in_headers_copy());
}

TEST_F(S3RequestObjectTest, ReturnsValidHeaderValue) {
  std::map<std::string, std::string> input_headers;
  input_headers["Content-Type"] = "application/xml";
  input_headers["Host"] = "kaustubh.s3.seagate.com";

  fake_in_headers(input_headers);

  EXPECT_EQ(std::string("application/xml"), request->get_header_value("Content-Type"));
}

TEST_F(S3RequestObjectTest, ReturnsValidHostHeaderValue) {
  std::map<std::string, std::string> input_headers;
  input_headers["Content-Type"] = "application/xml";
  input_headers["Host"] = "s3.seagate.com";

  fake_in_headers(input_headers);

  EXPECT_EQ(std::string("s3.seagate.com"), request->get_host_header());
}

TEST_F(S3RequestObjectTest, ReturnsValidContentLengthHeaderValue) {
  std::map<std::string, std::string> input_headers;
  input_headers["Content-Length"] = "852";
  input_headers["Host"] = "s3.seagate.com";

  fake_in_headers(input_headers);

  EXPECT_EQ(852, request->get_content_length());
}

TEST_F(S3RequestObjectTest, ReturnsValidContentBody) {
  std::string content = "Hello World";
  std::map<std::string, std::string> input_headers;
  input_headers["Content-Length"] = "11";

  fake_in_headers(input_headers);
  fake_buffer_in(content);

  EXPECT_EQ(content, request->get_full_body_content_as_string());
}

TEST_F(S3RequestObjectTest, ReturnsValidQueryStringValue) {
  fake_query_params("location=US&policy=test");

  EXPECT_EQ(std::string("US"), request->get_query_string_value("location"));
}

TEST_F(S3RequestObjectTest, ReturnsValidQueryStringNullValue) {
  fake_query_params("location=US&policy");

  EXPECT_EQ(std::string(""), request->get_query_string_value("policy"));
}

TEST_F(S3RequestObjectTest, HasValidQueryStringParam) {
  fake_query_params("location=US&policy=secure");

  EXPECT_TRUE(request->has_query_param_key("location"));
}

TEST_F(S3RequestObjectTest, MissingQueryStringParam) {
  fake_query_params("location=US&policy=secure");

  EXPECT_FALSE(request->has_query_param_key("acl"));
}

TEST_F(S3RequestObjectTest, SetsOutputHeader) {
  request->set_out_header_value("Content-Type", "application/xml");
  EXPECT_TRUE(evhtp_kvs_find_kv(ev_request->headers_out, "Content-Type") != NULL);
}

TEST_F(S3RequestObjectTest, ReturnsValidObjectURI) {
  std::string bucket = "s3project";
  std::string object = "member";
  request->set_bucket_name(bucket);
  request->set_object_name(object);
  EXPECT_EQ(std::string("s3project/member"), request->get_object_uri());
}

TEST_F(S3RequestObjectTest, SetsBucketName) {
  std::string bucket = "s3bucket";
  request->set_bucket_name(bucket);
  EXPECT_EQ(std::string("s3bucket"), request->get_bucket_name());
}

TEST_F(S3RequestObjectTest, SetsObjectName) {
  std::string object = "Makefile";
  request->set_object_name(object);
  EXPECT_EQ(std::string("Makefile"), request->get_object_name());
}

TEST_F(S3RequestObjectTest, SetsUserName) {
  std::string user = "kaustubh";
  request->set_user_name(user);
  EXPECT_EQ(std::string("kaustubh"), request->get_user_name());
}

TEST_F(S3RequestObjectTest, SetsUserID) {
  std::string user_id = "rajesh";
  request->set_user_id(user_id);
  EXPECT_EQ(std::string("rajesh"), request->get_user_id());
}

TEST_F(S3RequestObjectTest, SetsAccountName) {
  std::string account = "arjun";
  request->set_account_name(account);
  EXPECT_EQ(std::string("arjun"), request->get_account_name());
}

TEST_F(S3RequestObjectTest, SetsAccountID) {
  std::string account_id = "bikrant";
  request->set_account_id(account_id);
  EXPECT_EQ(std::string("bikrant"), request->get_account_id());
}

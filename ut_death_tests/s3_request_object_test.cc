#include "s3_request_object.h"
#include "gtest/gtest.h"
#include "mock_evhtp_wrapper.h"

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

// To use a test fixture, derive a class from testing::Test.
class S3RequestObjectTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3RequestObjectTest() {
    // placeholder evbase
    evbase = event_base_new();
    ev_request = evhtp_request_new(dummy_request_cb, evbase);
    mock_evhtp_obj_ptr = new MockEvhtpWrapper();
    request = new S3RequestObject(ev_request, mock_evhtp_obj_ptr);
  }

  ~S3RequestObjectTest() {
    delete request;
    event_base_free(evbase);
  }
  // Declares the variables your tests want to use.
  S3RequestObject *request;
  evhtp_request_t *ev_request;  // To fill test data
  evbase_t *evbase;
  MockEvhtpWrapper *mock_evhtp_obj_ptr;
};

TEST_F(S3RequestObjectTest, SettingContentLengthTwice) {
  EXPECT_CALL(*mock_evhtp_obj_ptr, http_header_new(_, _, _, _)).Times(1);
  EXPECT_CALL(*mock_evhtp_obj_ptr, http_headers_add_header(_, _)).Times(1);
  request->set_out_header_value("Content-Length", "0");
  ASSERT_DEATH(request->set_out_header_value("Content-Length", "0"), ".*");
}

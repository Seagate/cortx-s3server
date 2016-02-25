/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original creation date: 14-Jan-2016
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "s3_clovis_reader.h"
#include "mock_s3_request_object.h"
#include "mock_s3_clovis_wrapper.h"
#include "s3_callback_test_helpers.h"

using ::testing::_;
using ::testing::Return;

static void
dummy_request_cb(evhtp_request_t * req, void * arg) {
}

class S3ClovisReaderTest : public testing::Test {
  protected:
    S3ClovisReaderTest() {
      evbase = event_base_new();
      req = evhtp_request_new(dummy_request_cb, evbase);
      EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
      request_mock = std::make_shared<MockS3RequestObject> (req, evhtp_obj_ptr);
      s3_clovis_api = std::make_shared<MockS3Clovis>();
      clovis_reader_ptr = new S3ClovisReader(request_mock, s3_clovis_api);
    }

  ~S3ClovisReaderTest() {
     event_base_free(evbase);
     delete clovis_reader_ptr;
   }

  evbase_t *evbase;
  evhtp_request_t * req;
  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<MockS3Clovis> s3_clovis_api;
  S3ClovisReader *clovis_reader_ptr;
};

TEST_F(S3ClovisReaderTest, Constructor) {
  EXPECT_EQ(S3ClovisReaderOpState::start, clovis_reader_ptr->get_state());
  EXPECT_EQ(request_mock, clovis_reader_ptr->request);
  EXPECT_EQ(0, clovis_reader_ptr->iteration_index);
  EXPECT_EQ(0, clovis_reader_ptr->last_index);
}

// TODO add more tests when we have ability to mock clovis init
// Issue: Currently without clovis init we are not even able to Create
// clovis structures for unit testing.
// TEST_F(S3ClovisReaderTest, ReadObjectDataSuccessStatusAndSuccessCallback) {
//   S3CallBack callback_object;
//
//   // EXPECT_CALL(*s3_clovis_api, clovis_obj_init(_, _, _));
//   // EXPECT_CALL(*s3_clovis_api, clovis_obj_op(_, _, _, _, _, _, _));
//   // EXPECT_CALL(*s3_clovis_api, clovis_op_setup(_, _, _));
//   EXPECT_CALL(*s3_clovis_api, clovis_op_launch(_, _));
//   EXPECT_CALL(*request_mock, get_evbase())
//               .WillOnce(Return(evbase));
//
//   size_t num_of_blocks_to_read = 2;
//   clovis_reader_ptr->read_object_data(num_of_blocks_to_read,
//                                   std::bind(&S3CallBack::on_success, &callback_object),
//                                   std::bind(&S3CallBack::on_failed, &callback_object));
//
//   // Treat operation as successful.
//
//   // clovis calls this, we(=test) fake.
//   s3_clovis_op_stable(clovis_reader_ptr->reader_context->get_clovis_op_ctx()->ops[0]);
//   EXPECT_TRUE(clovis_reader_ptr->reader_context->get_op_status_for(0) == S3AsyncOpStatus::success);
//
//   // Main thread handler call this, we(=test) fake it
//   clovis_reader_ptr->read_object_data_successful();
//   EXPECT_TRUE(callback_object.success_called == FALSE);
//   EXPECT_TRUE(callback_object.fail_called == TRUE);
// }

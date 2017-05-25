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
 * Original author:  Abrarahmed Momin  <abrar.habib@seagate.com>
 * Original creation date: 14-Jan-2016
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_request_object.h"
#include "s3_callback_test_helpers.h"
#include "s3_clovis_layout.h"
#include "s3_clovis_reader.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

static void s3_test_clovis_obj_op(struct m0_clovis_obj *obj,
                                  enum m0_clovis_obj_opcode opcode,
                                  struct m0_indexvec *ext,
                                  struct m0_bufvec *data,
                                  struct m0_bufvec *attr, uint64_t mask,
                                  struct m0_clovis_op **op) {
  *op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
}

static void s3_test_free_clovis_op(struct m0_clovis_op *op) { free(op); }

static void s3_test_clovis_op_launch(struct m0_clovis_op **op, uint32_t nr,
                                     ClovisOpType type) {
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op[0]->op_datum;

  S3ClovisReaderContext *app_ctx =
      (S3ClovisReaderContext *)ctx->application_context;
  struct s3_clovis_op_context *op_ctx = app_ctx->get_clovis_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    s3_clovis_op_stable(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->op_count = 0;
}

static void s3_test_clovis_op_launch_fail(struct m0_clovis_op **op, uint32_t nr,
                                          ClovisOpType type) {
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op[0]->op_datum;

  S3ClovisReaderContext *app_ctx =
      (S3ClovisReaderContext *)ctx->application_context;
  struct s3_clovis_op_context *op_ctx = app_ctx->get_clovis_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    test_clovis_op->op_sm.sm_rc = -EPERM;
    s3_clovis_op_failed(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->op_count = 0;
}

static void s3_test_clovis_op_launch_fail_enoent(struct m0_clovis_op **op,
                                                 uint32_t nr,
                                                 ClovisOpType type) {
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op[0]->op_datum;

  S3ClovisReaderContext *app_ctx =
      (S3ClovisReaderContext *)ctx->application_context;
  struct s3_clovis_op_context *op_ctx = app_ctx->get_clovis_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    test_clovis_op->op_sm.sm_rc = -ENOENT;
    s3_clovis_op_failed(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->op_count = 0;
}

class S3ClovisReaderTest : public testing::Test {
 protected:
  S3ClovisReaderTest() {
    oid = {0x1ffff, 0x1ffff};
    layout_id =
        S3ClovisLayoutMap::get_instance()->get_best_layout_for_object_size();

    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_clovis_api_mock = std::make_shared<MockS3Clovis>();
    clovis_reader_ptr = std::make_shared<S3ClovisReader>(
        request_mock, oid, layout_id, s3_clovis_api_mock);
  }

  ~S3ClovisReaderTest() { event_base_free(evbase); }

  struct m0_uint128 oid;
  int layout_id;
  evbase_t *evbase;
  evhtp_request_t *req;
  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<MockS3Clovis> s3_clovis_api_mock;
  std::shared_ptr<S3ClovisReader> clovis_reader_ptr;
};

TEST_F(S3ClovisReaderTest, Constructor) {
  EXPECT_EQ(S3ClovisReaderOpState::start, clovis_reader_ptr->get_state());
  EXPECT_EQ(request_mock, clovis_reader_ptr->request);
  EXPECT_EQ(0, clovis_reader_ptr->iteration_index);
  EXPECT_EQ(0, clovis_reader_ptr->last_index);
}

TEST_F(S3ClovisReaderTest, ReadObjectDataTest) {
  S3CallBack s3clovisreader_callbackobj;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  EXPECT_TRUE(s3clovisreader_callbackobj.success_called);
  EXPECT_FALSE(s3clovisreader_callbackobj.fail_called);
}

TEST_F(S3ClovisReaderTest, ReadObjectDataSuccessful) {
  S3CallBack s3clovisreader_callbackobj;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  clovis_reader_ptr->read_object_data_successful();
  EXPECT_TRUE(clovis_reader_ptr->get_state() == S3ClovisReaderOpState::success);
  EXPECT_TRUE(s3clovisreader_callbackobj.success_called);
  EXPECT_FALSE(s3clovisreader_callbackobj.fail_called);
}

TEST_F(S3ClovisReaderTest, ReadObjectDataFailed) {
  S3CallBack s3clovisreader_callbackobj;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  clovis_reader_ptr->read_object_data_failed();
  EXPECT_TRUE(clovis_reader_ptr->get_state() == S3ClovisReaderOpState::failed);
  EXPECT_TRUE(s3clovisreader_callbackobj.fail_called);
  EXPECT_FALSE(s3clovisreader_callbackobj.success_called);
}

TEST_F(S3ClovisReaderTest, ReadObjectDataFailedMissing) {
  S3CallBack s3clovisreader_callbackobj;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail_enoent));

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  clovis_reader_ptr->read_object_data_failed();
  EXPECT_TRUE(clovis_reader_ptr->get_state() == S3ClovisReaderOpState::missing);
  EXPECT_TRUE(s3clovisreader_callbackobj.fail_called);
  EXPECT_FALSE(s3clovisreader_callbackobj.success_called);
}

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

static void s3_test_free_op(struct m0_clovis_op *op) { free(op); }

class S3ClovisReaderTest : public testing::Test {
 protected:
  S3ClovisReaderTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_clovis_api_mock = std::make_shared<MockS3Clovis>();
    clovis_reader_ptr =
        std::make_shared<S3ClovisReader>(request_mock, s3_clovis_api_mock);
  }

  ~S3ClovisReaderTest() { event_base_free(evbase); }

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
  struct s3_clovis_op_context *op_ctx;
  struct s3_clovis_rw_op_context *rw_ctx;
  pthread_t tid;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  op_ctx = clovis_reader_ptr->reader_context->get_clovis_op_ctx();
  rw_ctx = clovis_reader_ptr->reader_context->get_clovis_rw_op_ctx();

  EXPECT_TRUE(op_ctx != NULL);
  EXPECT_TRUE(rw_ctx != NULL);
  EXPECT_TRUE(op_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(op_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(op_ctx->cbs->oop_failed != NULL);

  struct s3_clovis_context_obj *obj_ctx =
      (struct s3_clovis_context_obj *)op_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)obj_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)op_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  op_ctx->op_count = 0;

  s3_test_free_op(op_ctx->ops[0]);
}

TEST_F(S3ClovisReaderTest, ReadObjectDataSuccessful) {
  S3CallBack s3clovisreader_callbackobj;
  struct s3_clovis_op_context *op_ctx;
  struct s3_clovis_rw_op_context *rw_ctx;
  pthread_t tid;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  op_ctx = clovis_reader_ptr->reader_context->get_clovis_op_ctx();
  rw_ctx = clovis_reader_ptr->reader_context->get_clovis_rw_op_ctx();

  EXPECT_TRUE(op_ctx != NULL);
  EXPECT_TRUE(rw_ctx != NULL);
  EXPECT_TRUE(op_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(op_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(op_ctx->cbs->oop_failed != NULL);

  struct s3_clovis_context_obj *obj_ctx =
      (struct s3_clovis_context_obj *)op_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)obj_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)op_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  op_ctx->op_count = 0;

  clovis_reader_ptr->read_object_data_successful();
  EXPECT_TRUE(clovis_reader_ptr->get_state() == S3ClovisReaderOpState::success);
  EXPECT_TRUE(s3clovisreader_callbackobj.success_called == TRUE);
  EXPECT_TRUE(s3clovisreader_callbackobj.fail_called == FALSE);

  s3_test_free_op(op_ctx->ops[0]);
}

TEST_F(S3ClovisReaderTest, ReadObjectDataFailed) {
  S3CallBack s3clovisreader_callbackobj;
  struct s3_clovis_op_context *op_ctx;
  struct s3_clovis_rw_op_context *rw_ctx;
  pthread_t tid;

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  size_t num_of_blocks_to_read = 2;
  clovis_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3clovisreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3clovisreader_callbackobj));

  op_ctx = clovis_reader_ptr->reader_context->get_clovis_op_ctx();
  rw_ctx = clovis_reader_ptr->reader_context->get_clovis_rw_op_ctx();

  EXPECT_TRUE(op_ctx != NULL);
  EXPECT_TRUE(rw_ctx != NULL);
  EXPECT_TRUE(op_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(op_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(op_ctx->cbs->oop_failed != NULL);

  struct s3_clovis_context_obj *obj_ctx =
      (struct s3_clovis_context_obj *)op_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)obj_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  op_ctx->ops[0]->op_sm.sm_rc = -ENOENT;
  pthread_create(&tid, NULL, async_fail_call, (void *)op_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::failed);
  op_ctx->op_count = 0;

  clovis_reader_ptr->read_object_data_failed();
  EXPECT_TRUE(clovis_reader_ptr->get_state() == S3ClovisReaderOpState::missing);
  EXPECT_TRUE(s3clovisreader_callbackobj.success_called == FALSE);
  EXPECT_TRUE(s3clovisreader_callbackobj.fail_called == TRUE);

  s3_test_free_op(op_ctx->ops[0]);
}

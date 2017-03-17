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
 * Original creation date: 27-Nov-2015
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>

#include "s3_callback_test_helpers.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_option.h"

#include "mock_s3_clovis_kvs_writer.h"
#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_request_object.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

int s3_test_alloc_op(struct m0_clovis_entity *entity,
                     struct m0_clovis_op **op) {
  *op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
  return 0;
}

int s3_test_clovis_idx_op(struct m0_clovis_idx *idx,
                          enum m0_clovis_idx_opcode opcode,
                          struct m0_bufvec *keys, struct m0_bufvec *vals,
                          int *rcs, unsigned int flags,
                          struct m0_clovis_op **op) {
  *op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
  return 0;
}

void s3_test_free_op(struct m0_clovis_op *op) { free(op); }

class S3ClovisKvsWritterTest : public testing::Test {
 protected:
  S3ClovisKvsWritterTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3clovis = std::make_shared<MockS3Clovis>();
    ptr_mock_cloviskvs_writer = std::make_shared<MockS3ClovisKVSWriter>(
        ptr_mock_request, ptr_mock_s3clovis);
  }

  ~S3ClovisKvsWritterTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3clovis;
  std::shared_ptr<MockS3ClovisKVSWriter> ptr_mock_cloviskvs_writer;
  S3ClovisKVSWriter *p_cloviskvs;
};

TEST_F(S3ClovisKvsWritterTest, Constructor) {
  EXPECT_EQ(S3ClovisKVSWriterOpState::start,
            ptr_mock_cloviskvs_writer->get_state());
  EXPECT_EQ(ptr_mock_cloviskvs_writer->request, ptr_mock_request);
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexSuccessCallback) {
  pthread_t tid;
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_mock_cloviskvs_writer->create_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) != 0);

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;

  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);

  s3_test_free_op(idx_ctx->ops[0]);
  // To ensure cleanup doesn't call fini
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexFailCallback) {
  pthread_t tid;

  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_mock_cloviskvs_writer->create_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();

  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) != 0);

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;

  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_fail_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::failed);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexFailEmpty) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->create_index(
      "", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) == 0);

  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexSuccess) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->create_index(
      "BUCKET/seagate_bucket",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->create_index_successful();
  EXPECT_TRUE(ptr_mock_cloviskvs_writer->get_state() ==
              S3ClovisKVSWriterOpState::saved);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == FALSE);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexFail) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));

  ptr_mock_cloviskvs_writer->create_index(
      "BUCKET/seagate_bucket",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->create_index_failed();
  EXPECT_TRUE(ptr_mock_cloviskvs_writer->get_state() ==
              S3ClovisKVSWriterOpState::failed);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == TRUE);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValSuccessCallback) {
  pthread_t tid;

  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_mock_cloviskvs_writer->put_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) != 0);
  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ("3kfile", key_str.c_str());
  S3Option *option_instance = S3Option::get_instance();

  if (!option_instance->get_delete_kv_before_put()) {
    std::string value_str((char *)kvs_ctx->values->ov_buf[0],
                          kvs_ctx->values->ov_vec.v_count[0]);
    EXPECT_STREQ(
        "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"3kfile\"}",
        value_str.c_str());
  }
  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;

  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValFailCallback) {
  pthread_t tid;

  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_mock_cloviskvs_writer->put_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      "{\"ACL\":\"\",\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":"
      "\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->writer_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) != 0);

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;

  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_fail_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::failed);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValFailEmpty) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->put_keyval(
      "", "", "", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) == 0);

  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValSuccess) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->put_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      "{\"ACL\":\"\",\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":"
      "\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->create_index_successful();
  EXPECT_TRUE(ptr_mock_cloviskvs_writer->get_state() ==
              S3ClovisKVSWriterOpState::saved);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == FALSE);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValFail) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->put_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      "{\"ACL\":\"\",\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":"
      "\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->create_index_failed();
  EXPECT_TRUE(ptr_mock_cloviskvs_writer->get_state() ==
              S3ClovisKVSWriterOpState::failed);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == TRUE);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValSuccessCallback) {
  pthread_t tid;

  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_mock_cloviskvs_writer->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) != 0);
  EXPECT_STREQ("3kfile", (char *)kvs_ctx->keys->ov_buf[0]);

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;

  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValFailCallback) {
  pthread_t tid;

  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_mock_cloviskvs_writer->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) != 0);

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;

  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_fail_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::failed);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValFailEmpty) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->delete_keyval(
      "", "", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);
  EXPECT_TRUE((ptr_mock_cloviskvs_writer->id.u_lo |
               ptr_mock_cloviskvs_writer->id.u_hi) == 0);
  EXPECT_STREQ("", (char *)kvs_ctx->keys->ov_buf[0]);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValSuccess) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->create_index_successful();
  EXPECT_TRUE(ptr_mock_cloviskvs_writer->get_state() ==
              S3ClovisKVSWriterOpState::saved);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == FALSE);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValFail) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  ptr_mock_cloviskvs_writer->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  idx_ctx = ptr_mock_cloviskvs_writer->writer_context->get_clovis_idx_op_ctx();
  ptr_mock_cloviskvs_writer->create_index_failed();
  EXPECT_TRUE(ptr_mock_cloviskvs_writer->get_state() ==
              S3ClovisKVSWriterOpState::failed);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == TRUE);
  s3_test_free_op(idx_ctx->ops[0]);
  idx_ctx->idx_count = 0;
}

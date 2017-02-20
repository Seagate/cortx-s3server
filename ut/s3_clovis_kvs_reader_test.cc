/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original author:  Abrarahmed Momin  <abrar.habib@seagate.com>
 * Original creation date: 30-Jan-2017
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>

#include "s3_callback_test_helpers.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_option.h"

#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_request_object.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

int s3_kvs_test_clovis_idx_op(struct m0_clovis_idx *idx,
                              enum m0_clovis_idx_opcode opcode,
                              struct m0_bufvec *keys, struct m0_bufvec *vals,
                              int *rcs, struct m0_clovis_op **op) {
  *op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
  return 0;
}

void s3_kvs_test_free_op(struct m0_clovis_op *op) { free(op); }

class S3ClovisKvsReaderTest : public testing::Test {
 protected:
  S3ClovisKvsReaderTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_s3request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3clovis = std::make_shared<MockS3Clovis>();
    ptr_cloviskvs_reader = std::make_shared<S3ClovisKVSReader>(
        ptr_mock_s3request, ptr_mock_s3clovis);
    index_oid = {0ULL, 0ULL};
  }

  ~S3ClovisKvsReaderTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  std::shared_ptr<MockS3RequestObject> ptr_mock_s3request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3clovis;
  std::shared_ptr<S3ClovisKVSReader> ptr_cloviskvs_reader;

  struct m0_uint128 index_oid;
  std::string test_key;
  std::string index_name;
  int nr_kvp;
};

TEST_F(S3ClovisKvsReaderTest, Constructor) {
  EXPECT_EQ(ptr_cloviskvs_reader->get_state(), S3ClovisKVSReaderOpState::start);
  EXPECT_EQ(ptr_cloviskvs_reader->request, ptr_mock_s3request);
  EXPECT_EQ(ptr_cloviskvs_reader->last_value, "");
  EXPECT_EQ(ptr_cloviskvs_reader->iteration_index, 0);
  EXPECT_EQ(ptr_cloviskvs_reader->last_result_keys_values.empty(), true);
}

TEST_F(S3ClovisKvsReaderTest, GetKeyvalTest) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  idx_ctx->idx_count = 0;

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

TEST_F(S3ClovisKvsReaderTest, GetKeyvalTestEmpty) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  test_key = "";  // Empty key string

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  idx_ctx->idx_count = 0;

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

TEST_F(S3ClovisKvsReaderTest, GetKeyvalSuccessfulTest) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  idx_ctx->idx_count = 0;

  ptr_cloviskvs_reader->get_keyval_successful();
  EXPECT_TRUE(ptr_cloviskvs_reader->get_state() ==
              S3ClovisKVSReaderOpState::present);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == FALSE);

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

TEST_F(S3ClovisKvsReaderTest, GetKeyvalFailedTest) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  idx_ctx->ops[0]->op_sm.sm_rc = -ENOENT;
  pthread_create(&tid, NULL, async_fail_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::failed);
  idx_ctx->idx_count = 0;

  ptr_cloviskvs_reader->get_keyval_failed();
  EXPECT_TRUE(ptr_cloviskvs_reader->get_state() ==
              S3ClovisKVSReaderOpState::missing);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == TRUE);

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

TEST_F(S3ClovisKvsReaderTest, NextKeyvalTest) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->next_keyval(
      index_oid, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  idx_ctx->idx_count = 0;

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

TEST_F(S3ClovisKvsReaderTest, NextKeyvalSuccessfulTest) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  index_name = "ACCOUNT/s3ut_test";
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->next_keyval(
      index_name, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  pthread_create(&tid, NULL, async_success_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::success);
  idx_ctx->idx_count = 0;

  ptr_cloviskvs_reader->next_keyval_successful();
  EXPECT_TRUE(ptr_cloviskvs_reader->get_state() ==
              S3ClovisKVSReaderOpState::present);
  EXPECT_EQ(ptr_cloviskvs_reader->last_result_keys_values.size(), 1);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == FALSE);

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

TEST_F(S3ClovisKvsReaderTest, NextKeyvalFailedTest) {
  S3CallBack s3cloviskvscallbackobj;
  struct s3_clovis_idx_op_context *idx_ctx;
  struct s3_clovis_kvs_op_context *kvs_ctx;
  pthread_t tid;

  index_name = "ACCOUNT/s3ut_test";
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _));
  S3Option::get_instance()->set_eventbase(evbase);

  ptr_cloviskvs_reader->next_keyval(
      index_name, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  idx_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_idx_op_ctx();
  kvs_ctx = ptr_cloviskvs_reader->reader_context->get_clovis_kvs_op_ctx();
  EXPECT_TRUE(idx_ctx->ops[0]->op_datum != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_stable != NULL);
  EXPECT_TRUE(idx_ctx->cbs->oop_failed != NULL);

  std::string key_str((char *)kvs_ctx->keys->ov_buf[0],
                      kvs_ctx->keys->ov_vec.v_count[0]);
  EXPECT_STREQ(test_key.c_str(), key_str.c_str());

  struct s3_clovis_context_obj *op_ctx =
      (struct s3_clovis_context_obj *)idx_ctx->ops[0]->op_datum;
  S3AsyncOpContextBase *ctx =
      (S3AsyncOpContextBase *)op_ctx->application_context;

  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::unknown);
  idx_ctx->ops[0]->op_sm.sm_rc = -ENOENT;
  pthread_create(&tid, NULL, async_fail_call, (void *)idx_ctx);
  pthread_join(tid, NULL);
  EXPECT_TRUE(ctx->get_op_status_for(0) == S3AsyncOpStatus::failed);
  idx_ctx->idx_count = 0;

  ptr_cloviskvs_reader->next_keyval_failed();
  EXPECT_TRUE(ptr_cloviskvs_reader->get_state() ==
              S3ClovisKVSReaderOpState::missing);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called == TRUE);

  s3_kvs_test_free_op(idx_ctx->ops[0]);
}

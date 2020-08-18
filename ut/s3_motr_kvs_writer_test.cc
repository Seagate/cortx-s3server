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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>

#include "s3_callback_test_helpers.h"
#include "s3_motr_kvs_writer.h"
#include "s3_option.h"
#include "s3_ut_common.h"

#include "mock_s3_motr_kvs_writer.h"
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_request_object.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::AtLeast;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

int s3_test_alloc_op(struct m0_entity *entity, struct m0_op **op) {
  *op = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  return 0;
}

int s3_test_alloc_sync_op(struct m0_op **sync_op) {
  *sync_op = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  return 0;
}

int s3_test_motr_idx_op(struct m0_idx *idx, enum m0_idx_opcode opcode,
                        struct m0_bufvec *keys, struct m0_bufvec *vals,
                        int *rcs, unsigned int flags, struct m0_op **op) {
  *op = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  return 0;
}

void s3_test_free_motr_op(struct m0_op *op) { free(op); }

static void s3_test_motr_op_launch(uint64_t, struct m0_op **op, uint32_t nr,
                                   MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3AsyncMotrKVSWriterContext *app_ctx =
      (S3AsyncMotrKVSWriterContext *)ctx->application_context;
  struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_stable(test_motr_op);
    s3_test_free_motr_op(test_motr_op);
  }
  op_ctx->op_count = 0;
  *op = NULL;
}

static void s3_test_motr_op_launch_fail(uint64_t, struct m0_op **op,
                                        uint32_t nr, MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3AsyncMotrKVSWriterContext *app_ctx =
      (S3AsyncMotrKVSWriterContext *)ctx->application_context;
  struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_failed(test_motr_op);
    s3_test_free_motr_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

static void s3_test_motr_op_launch_fail_exists(uint64_t, struct m0_op **op,
                                               uint32_t nr, MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3AsyncMotrKVSWriterContext *app_ctx =
      (S3AsyncMotrKVSWriterContext *)ctx->application_context;
  struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_failed(test_motr_op);
    s3_test_free_motr_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

class S3MotrKVSWritterTest : public testing::Test {
 protected:
  S3MotrKVSWritterTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3motr = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*ptr_mock_s3motr, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(0));

    action_under_test =
        std::make_shared<S3MotrKVSWriter>(ptr_mock_request, ptr_mock_s3motr);
    oid = {0xffff, 0xfff1f};
  }

  ~S3MotrKVSWritterTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  struct m0_uint128 oid;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3motr;
  std::shared_ptr<S3MotrKVSWriter> action_under_test;
  S3MotrKVSWriter *p_motrkvs;
};

TEST_F(S3MotrKVSWritterTest, Constructor) {
  EXPECT_EQ(S3MotrKVSWriterOpState::start, action_under_test->get_state());
  EXPECT_EQ(action_under_test->request, ptr_mock_request);
  EXPECT_TRUE(action_under_test->idx_ctx == nullptr);
  EXPECT_EQ(0, action_under_test->oid_list.size());
}

TEST_F(S3MotrKVSWritterTest, CleanupContexts) {
  action_under_test->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  action_under_test->idx_ctx->idx =
      (struct m0_idx *)calloc(2, sizeof(struct m0_idx));
  action_under_test->idx_ctx->idx_count = 2;
  action_under_test->idx_ctx->n_initialized_contexts = 1;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  action_under_test->clean_up_contexts();
  EXPECT_TRUE(action_under_test->sync_context == nullptr);
  EXPECT_TRUE(action_under_test->writer_context == nullptr);
  EXPECT_TRUE(action_under_test->idx_ctx == nullptr);
}

TEST_F(S3MotrKVSWritterTest, CreateIndex) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  action_under_test->create_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_EQ(1, action_under_test->oid_list.size());
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, CreateIndexIdxPresent) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  action_under_test->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  action_under_test->idx_ctx->idx =
      (struct m0_idx *)calloc(2, sizeof(struct m0_idx));
  action_under_test->idx_ctx->idx_count = 2;
  action_under_test->idx_ctx->n_initialized_contexts = 2;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(3);

  action_under_test->create_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_EQ(1, action_under_test->oid_list.size());
  EXPECT_EQ(1, action_under_test->idx_ctx->idx_count);
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, CreateIndexSuccessful) {
  S3CallBack s3motrkvscallbackobj;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  action_under_test->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  action_under_test->idx_ctx->idx =
      (struct m0_idx *)calloc(1, sizeof(struct m0_idx));
  action_under_test->idx_ctx->idx_count = 1;
  action_under_test->idx_ctx->n_initialized_contexts = 1;

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj);

  action_under_test->create_index_successful();

  EXPECT_EQ(S3MotrKVSWriterOpState::created, action_under_test->get_state());
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, CreateIndexEntityCreateFailed) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_create(_, _)).WillOnce(Return(-1));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);

  action_under_test->create_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_EQ(1, action_under_test->oid_list.size());
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_EQ(S3MotrKVSWriterOpState::failed_to_launch,
            action_under_test->get_state());
}

TEST_F(S3MotrKVSWritterTest, CreateIndexFail) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  action_under_test->create_index(
      "BUCKET/seagate_bucket",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));
  action_under_test->create_index_failed();

  EXPECT_EQ(S3MotrKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
}

TEST_F(S3MotrKVSWritterTest, CreateIndexFailExists) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail_exists));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-EEXIST));
  action_under_test->create_index(
      "BUCKET/seagate_bucket",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));
  action_under_test->create_index_failed();

  EXPECT_EQ(S3MotrKVSWriterOpState::exists, action_under_test->get_state());
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
}

TEST_F(S3MotrKVSWritterTest, PutKeyVal) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  action_under_test->put_keyval(
      oid, "3kfile",
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, PutKeyValSuccessful) {
  S3CallBack s3motrkvscallbackobj;

  action_under_test->writer_context.reset(
      new S3AsyncMotrKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj);

  action_under_test->put_keyval_successful();

  EXPECT_EQ(S3MotrKVSWriterOpState::created, action_under_test->get_state());
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, PutKeyValFailed) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  action_under_test->writer_context.reset(new S3AsyncMotrKVSWriterContext(
      ptr_mock_request, NULL, NULL, 1, ptr_mock_s3motr));
  action_under_test->put_keyval(
      oid, "3kfile",
      "{\"ACL\":\"\",\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":"
      "\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));
  action_under_test->put_keyval_failed();

  EXPECT_EQ(S3MotrKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelKeyVal) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  action_under_test->delete_keyval(
      oid, "3kfile", std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelKeyValSuccess) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*ptr_mock_s3motr, motr_sync_op_init(_))
      .WillRepeatedly(Invoke(s3_test_alloc_sync_op));

  action_under_test->delete_keyval(
      oid, "3kfile", std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));
  action_under_test->delete_keyval_successful();

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelKeyValFailed) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  action_under_test->delete_keyval(
      oid, "3kfile", std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  action_under_test->delete_keyval_failed();
  EXPECT_EQ(S3MotrKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelIndex) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_delete(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_open(_, _));

  action_under_test->delete_index(
      oid, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelIndexIdxPresent) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_delete(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_open(_, _));

  action_under_test->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  action_under_test->idx_ctx->idx =
      (struct m0_idx *)calloc(2, sizeof(struct m0_idx));
  action_under_test->idx_ctx->idx_count = 2;
  action_under_test->idx_ctx->n_initialized_contexts = 2;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(3);

  action_under_test->delete_index(
      oid, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelIndexEntityDeleteFailed) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_delete(_, _)).WillOnce(Return(-1));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_entity_open(_, _));

  action_under_test->delete_index(
      oid, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, DelIndexFailed) {
  S3CallBack s3motrkvscallbackobj;
  action_under_test->writer_context.reset(
      new S3AsyncMotrKVSWriterContext(ptr_mock_request, NULL, NULL));
  action_under_test->writer_context->ops_response[0].error_code = -ENOENT;

  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj);
  action_under_test->delete_index_failed();
  EXPECT_EQ(S3MotrKVSWriterOpState::missing, action_under_test->get_state());
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);

  s3motrkvscallbackobj.fail_called = false;
  action_under_test->writer_context->ops_response[0].error_code = -EACCES;
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj);
  action_under_test->delete_index_failed();
  EXPECT_EQ(S3MotrKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}

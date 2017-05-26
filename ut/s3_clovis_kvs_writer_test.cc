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
using ::testing::AtLeast;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

int s3_test_alloc_op(struct m0_clovis_entity *entity,
                     struct m0_clovis_op **op) {
  *op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
  return 0;
}

int s3_test_alloc_sync_op(struct m0_clovis_op **sync_op) {
  *sync_op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
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

void s3_test_free_clovis_op(struct m0_clovis_op *op) { free(op); }

static void s3_test_clovis_op_launch(struct m0_clovis_op **op, uint32_t nr,
                                     ClovisOpType type) {
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op[0]->op_datum;

  S3ClovisKVSWriterContext *app_ctx =
      (S3ClovisKVSWriterContext *)ctx->application_context;
  struct s3_clovis_idx_op_context *op_ctx = app_ctx->get_clovis_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    s3_clovis_op_stable(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->idx_count = 0;
  *op = NULL;
}

static void s3_test_clovis_op_launch_fail(struct m0_clovis_op **op, uint32_t nr,
                                          ClovisOpType type) {
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op[0]->op_datum;

  S3ClovisKVSWriterContext *app_ctx =
      (S3ClovisKVSWriterContext *)ctx->application_context;
  struct s3_clovis_idx_op_context *op_ctx = app_ctx->get_clovis_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    test_clovis_op->op_sm.sm_rc = -EPERM;
    s3_clovis_op_failed(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->idx_count = 0;
}

static void s3_test_clovis_op_launch_fail_exists(struct m0_clovis_op **op,
                                                 uint32_t nr,
                                                 ClovisOpType type) {
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op[0]->op_datum;

  S3ClovisKVSWriterContext *app_ctx =
      (S3ClovisKVSWriterContext *)ctx->application_context;
  struct s3_clovis_idx_op_context *op_ctx = app_ctx->get_clovis_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    test_clovis_op->op_sm.sm_rc = -EEXIST;
    s3_clovis_op_failed(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->idx_count = 0;
}

class S3ClovisKvsWritterTest : public testing::Test {
 protected:
  S3ClovisKvsWritterTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3clovis = std::make_shared<MockS3Clovis>();
    action_under_test = std::make_shared<S3ClovisKVSWriter>(ptr_mock_request,
                                                            ptr_mock_s3clovis);
  }

  ~S3ClovisKvsWritterTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3clovis;
  std::shared_ptr<S3ClovisKVSWriter> action_under_test;
  S3ClovisKVSWriter *p_cloviskvs;
};

TEST_F(S3ClovisKvsWritterTest, Constructor) {
  EXPECT_EQ(S3ClovisKVSWriterOpState::start, action_under_test->get_state());
  EXPECT_EQ(action_under_test->request, ptr_mock_request);
}

TEST_F(S3ClovisKvsWritterTest, CreateIndex) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_entity_add(_, _));

  action_under_test->create_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexSuccessful) {
  S3CallBack s3cloviskvscallbackobj;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_entity_add(_, _));
  action_under_test->writer_context.reset(
      new S3ClovisKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);

  action_under_test->create_index_successful();

  EXPECT_EQ(S3ClovisKVSWriterOpState::saved, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexFail) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));

  action_under_test->create_index(
      "BUCKET/seagate_bucket",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  action_under_test->create_index_failed();

  EXPECT_EQ(S3ClovisKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, CreateIndexFailExists) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail_exists));

  action_under_test->create_index(
      "BUCKET/seagate_bucket",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  action_under_test->create_index_failed();

  EXPECT_EQ(S3ClovisKVSWriterOpState::exists, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncIndex) {
  S3CallBack s3cloviskvscallbackobj;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_entity_add(_, _));
  action_under_test->writer_context.reset(
      new S3ClovisKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);

  action_under_test->sync_index(
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  EXPECT_EQ(S3ClovisKVSWriterOpState::saved, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncIndexSuccessful) {
  S3CallBack s3cloviskvscallbackobj;
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->sync_index_successful();
  EXPECT_EQ(S3ClovisKVSWriterOpState::saved, action_under_test->state);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncIndexFailedMissingMetadata) {
  S3CallBack s3cloviskvscallbackobj;
  action_under_test->sync_context.reset(
      new S3ClovisKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);
  action_under_test->sync_context->ops_response[0].error_code = -ENOENT;
  action_under_test->sync_index_failed();

  EXPECT_EQ(S3ClovisKVSWriterOpState::missing, action_under_test->state);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncIndexFailedFailedMetadata) {
  S3CallBack s3cloviskvscallbackobj;
  action_under_test->sync_context.reset(
      new S3ClovisKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);
  action_under_test->sync_context->ops_response[0].error_code = -EACCES;
  action_under_test->sync_index_failed();

  EXPECT_EQ(S3ClovisKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, PutKeyVal) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _));

  action_under_test->put_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValEmpty) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _));

  action_under_test->put_keyval(
      "", "", "", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValSuccessful) {
  S3CallBack s3cloviskvscallbackobj;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _));
  action_under_test->writer_context.reset(
      new S3ClovisKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);

  action_under_test->put_keyval_successful();

  EXPECT_EQ(S3ClovisKVSWriterOpState::saved, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, PutKeyValFailed) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));
  action_under_test->put_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      "{\"ACL\":\"\",\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":"
      "\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  action_under_test->put_keyval_failed();

  EXPECT_EQ(S3ClovisKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncKeyVal) {
  S3CallBack s3cloviskvscallbackobj;
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _));
  action_under_test->writer_context.reset(
      new S3ClovisKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);

  action_under_test->sync_keyval(
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  EXPECT_EQ(S3ClovisKVSWriterOpState::saved, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncKeyvalSuccessful) {
  S3CallBack s3cloviskvscallbackobj;
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj);
  action_under_test->sync_keyval_successful();
  EXPECT_EQ(S3ClovisKVSWriterOpState::saved, action_under_test->state);
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, SyncKeyValFailed) {
  S3CallBack s3cloviskvscallbackobj;
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj);
  action_under_test->sync_keyval_failed();

  EXPECT_EQ(S3ClovisKVSWriterOpState::failed, action_under_test->state);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
}

TEST_F(S3ClovisKvsWritterTest, DelKeyVal) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _));
  action_under_test->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValEmpty) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _));
  action_under_test->delete_keyval(
      "", "", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValSuccess) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillRepeatedly(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_add(_, _)).Times(AtLeast(1));
  ;
  action_under_test->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));
  action_under_test->delete_keyval_successful();

  EXPECT_EQ(S3ClovisKVSWriterOpState::deleted, action_under_test->get_state());
  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, DelKeyValFailed) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_idx_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));

  action_under_test->delete_keyval(
      "BUCKET/seagate_bucket", "3kfile",
      std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  action_under_test->delete_keyval_failed();
  EXPECT_EQ(S3ClovisKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_FALSE(s3cloviskvscallbackobj.success_called);
  EXPECT_TRUE(s3cloviskvscallbackobj.fail_called);
}

TEST_F(S3ClovisKvsWritterTest, DelIndex) {
  S3CallBack s3cloviskvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3clovis, clovis_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_entity_delete(_, _))
      .WillOnce(Invoke(s3_test_alloc_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_launch(_, _, _))
      .WillRepeatedly(Invoke(s3_test_clovis_op_launch));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_op_init(_))
      .WillOnce(Invoke(s3_test_alloc_sync_op));
  EXPECT_CALL(*ptr_mock_s3clovis, clovis_sync_entity_add(_, _));

  action_under_test->delete_index(
      "TestIndex", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj));

  EXPECT_TRUE(s3cloviskvscallbackobj.success_called);
  EXPECT_FALSE(s3cloviskvscallbackobj.fail_called);
}

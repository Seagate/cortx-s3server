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
using ::testing::InSequence;

struct m0_op **g_test_motr_op = NULL;
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

#if 0
static void s3_test_motr_op_parallel_launch(uint64_t, struct m0_op **op,
                                            uint32_t nr, MotrOpType type) {
  // struct s3_motr_context_obj *ctx =
  //  (struct s3_motr_context_obj *)op[0]->op_datum;

  // S3AsyncMotrKVSWriterContext *app_ctx =
  // (S3AsyncMotrKVSWriterContext *)ctx->application_context;
  // struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    g_test_motr_op = op;
    s3_motr_op_stable(test_motr_op);
    // s3_test_free_motr_op(test_motr_op);
  }
}
#endif

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
    p_motrkvs = nullptr;
  }

 public:
#if 0
  void dummy_parallel_kv_callback(unsigned int processed_count) {
    if (action_under_test) {
      // free App context
      if (g_test_motr_op != NULL) {
        s3_test_free_motr_op(*g_test_motr_op);
        *g_test_motr_op = NULL;
      }
      action_under_test->parallel_put_kv_successful(processed_count);
    }
  }
#endif
  void dummy_put_kv_success_cb(void) { put_all_kv_completed_success = true; }
  void dummy_put_kv_failed_cb(void) { put_all_kv_completed_success = false; }

  ~S3MotrKVSWritterTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  struct m0_uint128 oid;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3motr;
  std::shared_ptr<S3MotrKVSWriter> action_under_test;
  S3MotrKVSWriter *p_motrkvs;
  bool put_all_kv_completed_success = false;
};

TEST_F(S3MotrKVSWritterTest, Constructor) {
  EXPECT_EQ(S3MotrKVSWriterOpState::start, action_under_test->get_state());
  EXPECT_EQ(action_under_test->request, ptr_mock_request);
  EXPECT_TRUE(action_under_test->idx_ctx == nullptr);
  EXPECT_EQ(0, action_under_test->idx_los.size());
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

  EXPECT_EQ(1, action_under_test->idx_los.size());
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

  EXPECT_EQ(1, action_under_test->idx_los.size());
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
  action_under_test->idx_los.resize(1);

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

  EXPECT_EQ(1, action_under_test->idx_los.size());
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
      {oid}, "3kfile",
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, PutPartialKeyVal) {
  S3CallBack s3motrkvscallbackobj;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  std::map<std::string, std::string> key_values = {{"key1", "vlv1"},
                                                   {"key2", "vlv2"}};
  action_under_test->put_partial_keyval(
      {oid}, key_values,
      std::bind(&S3CallBack::on_success_with_arg, &s3motrkvscallbackobj,
                std::placeholders::_1),
      std::bind(&S3CallBack::on_failed_with_arg, &s3motrkvscallbackobj,
                std::placeholders::_1),
      0, 1);

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

TEST_F(S3MotrKVSWritterTest, ParallelPutKeyValSuccessful) {
  // Total 3 keys
  std::map<std::string, std::string> put_kv_list;
  put_kv_list["key1"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key1\"}";
  put_kv_list["key2"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key2\"}";
  put_kv_list["key3"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key3\"}";

  action_under_test->handler_on_success =
      std::bind(&S3MotrKVSWritterTest::dummy_put_kv_success_cb, this);
  put_all_kv_completed_success = false;
  // Sub test1 - Already processed 1 kv
  action_under_test->kv_list = put_kv_list;
  // Max number of parallel put kvs in-flight
  action_under_test->max_parallel_kv = 2;
  // Two are in-flight
  action_under_test->kvop_keys_in_flight = 2;
  // Already processed 1 kv
  action_under_test->next_key_offset = 1;
  // So far, no failure
  action_under_test->atleast_one_failed = false;
  // Below is the callback for 2nd put kv success
  action_under_test->parallel_put_kv_successful(1);
  EXPECT_EQ(action_under_test->kvop_keys_in_flight, 1);
  EXPECT_EQ(action_under_test->next_key_offset, 2);
  EXPECT_FALSE(put_all_kv_completed_success);

  // Sub test2 - Already processed 2 kv
  action_under_test->next_key_offset = 2;
  // One in-flight
  action_under_test->kvop_keys_in_flight = 1;
  // Below is the callback for 3rd (and last) put kv success
  action_under_test->parallel_put_kv_successful(1);
  EXPECT_EQ(action_under_test->kvop_keys_in_flight, 0);
  EXPECT_EQ(action_under_test->next_key_offset, 3);
  EXPECT_TRUE(put_all_kv_completed_success);
}

TEST_F(S3MotrKVSWritterTest, ParallelPutKeyValFail) {
  // Total 3 keys
  std::map<std::string, std::string> put_kv_list;
  put_kv_list["key1"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key1\"}";
  put_kv_list["key2"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key2\"}";
  put_kv_list["key3"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key3\"}";

  // Max number of parallel put kvs in-flight
  action_under_test->max_parallel_kv = 2;
  action_under_test->handler_on_success =
      std::bind(&S3MotrKVSWritterTest::dummy_put_kv_success_cb, this);
  action_under_test->handler_on_failed =
      std::bind(&S3MotrKVSWritterTest::dummy_put_kv_failed_cb, this);

  put_all_kv_completed_success = true;
  // Sub test1 - First put kv fails, second is in progress
  action_under_test->next_key_offset = 0;
  // Total two put kv are in-flight
  action_under_test->kvop_keys_in_flight = 2;
  action_under_test->atleast_one_failed = false;
  action_under_test->kv_list = put_kv_list;

  // Below is the callback for 1st put kv faliure
  action_under_test->parallel_put_kv_failed(1);
  EXPECT_EQ(action_under_test->kvop_keys_in_flight, 1);
  EXPECT_EQ(action_under_test->next_key_offset, 1);
  EXPECT_TRUE(action_under_test->atleast_one_failed);
  // No failure cb called, so 'put_all_kv_completed_success' does not change
  EXPECT_TRUE(put_all_kv_completed_success);

  // Sub test2 - Already processed 1 kv (with failure)
  // One remaining in-flight
  // Below is the callback for 2nd put kv success
  action_under_test->parallel_put_kv_successful(1);
  EXPECT_EQ(action_under_test->next_key_offset, 2);
  EXPECT_TRUE(action_under_test->atleast_one_failed);
  EXPECT_EQ(action_under_test->kvop_keys_in_flight, 0);
  EXPECT_FALSE(put_all_kv_completed_success);
}

#if 0
// PUT KV parallel mode success
TEST_F(S3MotrKVSWritterTest, PutParallelKeyValSuccessful) {
  S3CallBack s3motrkvscallbackobj;

  // 4 key/values => Few of below calls are made 4 times
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _)).Times(4);
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .Times(4)
      .WillRepeatedly(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _)).Times(4);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .Times(4)
      .WillRepeatedly(Invoke(s3_test_motr_op_parallel_launch));

  std::map<std::string, std::string> kv_list;
  kv_list["key1"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key1\"}";
  kv_list["key2"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key2\"}";
  kv_list["key3"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key3\"}";
  kv_list["key4"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key4\"}";

  // Set/override parallel put kv callback in test pointer
  action_under_test->on_success_for_parallelkv =
      std::bind(&S3MotrKVSWritterTest::dummy_parallel_kv_callback, this,
                std::placeholders::_1);
  action_under_test->put_keyval(
      {oid}, kv_list, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj), true);

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

// PUT KV parallel mode failure
TEST_F(S3MotrKVSWritterTest, PutParallelKeyValOneFailed) {
  S3CallBack s3motrkvscallbackobj;

  // 5 key/values => Few of below calls are made 5 times
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .Times(5)
      .WillRepeatedly(Invoke(s3_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _)).Times(5);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .Times(5)
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  {
    // Motr operation is success for first three times, then it fails
    InSequence s;
    EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
        .Times(3)
        .WillRepeatedly(Invoke(s3_test_motr_op_launch))
        .Times(2)
        .WillRepeatedly(Invoke(s3_test_motr_op_launch_fail));
  }
  // action_under_test->parallel_run = false;
  std::map<std::string, std::string> kv_list;
  kv_list["key1"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key1\"}";
  kv_list["key2"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key2\"}";
  kv_list["key3"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key3\"}";
  kv_list["key4"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key4\"}";
  kv_list["key5"] =
      "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"key5\"}";

  action_under_test->put_keyval(
      {oid}, kv_list, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj), true);

  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}
#endif

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
      {oid}, "3kfile",
      "{\"ACL\":\"\",\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":"
      "\"3kfile\"}",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));
  action_under_test->put_keyval_failed();

  EXPECT_EQ(S3MotrKVSWriterOpState::failed, action_under_test->get_state());
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, PutPartialKeyValSuccessful) {
  S3CallBack s3motrkvscallbackobj;

  action_under_test->writer_context.reset(
      new S3AsyncMotrKVSWriterContext(ptr_mock_request, NULL, NULL));

  action_under_test->on_success_for_extends =
      std::bind(&S3CallBack::on_success_with_arg, &s3motrkvscallbackobj,
                std::placeholders::_1);
  action_under_test->on_failure_for_extends =
      std::bind(&S3CallBack::on_failed_with_arg, &s3motrkvscallbackobj,
                std::placeholders::_1);

  action_under_test->put_partial_keyval_successful();

  EXPECT_EQ(S3MotrKVSWriterOpState::created, action_under_test->get_state());
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSWritterTest, PutPartialKeyValFailed) {
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
  std::map<std::string, std::string> key_values = {{"key1", "vlv1"},
                                                   {"key2", "vlv2"}};
  action_under_test->put_partial_keyval(
      {oid}, key_values,
      std::bind(&S3CallBack::on_success_with_arg, &s3motrkvscallbackobj,
                std::placeholders::_1),
      std::bind(&S3CallBack::on_failed_with_arg, &s3motrkvscallbackobj,
                std::placeholders::_1),
      0, 1);
  action_under_test->put_partial_keyval_failed();

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
      {oid}, "3kfile",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
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
      {oid}, "3kfile",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
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
      {oid}, "3kfile",
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
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
      {oid}, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
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
      {oid}, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
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
      {oid}, std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
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

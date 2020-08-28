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
#include "s3_motr_kvs_reader.h"
#include "s3_option.h"

#include "mock_s3_motr_wrapper.h"
#include "mock_s3_request_object.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}
static enum m0_idx_opcode g_opcode;

int s3_kvs_test_motr_idx_op(struct m0_idx *idx, enum m0_idx_opcode opcode,
                            struct m0_bufvec *keys, struct m0_bufvec *vals,
                            int *rcs, unsigned int flags, struct m0_op **op) {
  *op = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  g_opcode = opcode;
  return 0;
}

void s3_kvs_test_free_op(struct m0_op *op) { free(op); }

static void s3_test_motr_op_launch(uint64_t, struct m0_op **op, uint32_t nr,
                                   MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3MotrKVSReaderContext *app_ctx =
      (S3MotrKVSReaderContext *)ctx->application_context;
  struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  if (M0_IC_NEXT == g_opcode) {
    // For M0_IC_NEXT op, if there are any keys to be returned to the
    // application, motr overwrites the input key buffer ptr.
    struct s3_motr_kvs_op_context *kvs_ctx = app_ctx->get_motr_kvs_op_ctx();
    std::string ret_key = "random";
    kvs_ctx->keys->ov_vec.v_count[0] = ret_key.length();
    kvs_ctx->keys->ov_buf[0] = malloc(ret_key.length());
    memcpy(kvs_ctx->keys->ov_buf[0], (void *)ret_key.c_str(), ret_key.length());
  }

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_stable(test_motr_op);
    s3_kvs_test_free_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

static void s3_test_motr_op_launch_fail(uint64_t, struct m0_op **op,
                                        uint32_t nr, MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3MotrKVSReaderContext *app_ctx =
      (S3MotrKVSReaderContext *)ctx->application_context;
  struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_failed(test_motr_op);
    s3_kvs_test_free_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

static void s3_test_motr_op_launch_fail_enoent(uint64_t, struct m0_op **op,
                                               uint32_t nr, MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3MotrKVSReaderContext *app_ctx =
      (S3MotrKVSReaderContext *)ctx->application_context;
  struct s3_motr_idx_op_context *op_ctx = app_ctx->get_motr_idx_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_failed(test_motr_op);
    s3_kvs_test_free_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

class S3MotrKVSReaderTest : public testing::Test {
 protected:
  S3MotrKVSReaderTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_s3request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3motr = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(0));
    ptr_motrkvs_reader =
        std::make_shared<S3MotrKVSReader>(ptr_mock_s3request, ptr_mock_s3motr);
    index_oid = {0ULL, 0ULL};
  }

  ~S3MotrKVSReaderTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  std::shared_ptr<MockS3RequestObject> ptr_mock_s3request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3motr;
  std::shared_ptr<S3MotrKVSReader> ptr_motrkvs_reader;

  struct m0_uint128 index_oid;
  std::string test_key;
  std::string index_name;
  int nr_kvp;
};

TEST_F(S3MotrKVSReaderTest, Constructor) {
  EXPECT_EQ(ptr_motrkvs_reader->get_state(), S3MotrKVSReaderOpState::start);
  EXPECT_EQ(ptr_motrkvs_reader->request, ptr_mock_s3request);
  EXPECT_EQ(ptr_motrkvs_reader->last_value, "");
  EXPECT_EQ(ptr_motrkvs_reader->iteration_index, 0);
  EXPECT_EQ(ptr_motrkvs_reader->last_result_keys_values.empty(), true);
  EXPECT_EQ(ptr_motrkvs_reader->idx_ctx, nullptr);
}

TEST_F(S3MotrKVSReaderTest, CleanupContexts) {
  ptr_motrkvs_reader->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  ptr_motrkvs_reader->idx_ctx->idx =
      (struct m0_idx *)calloc(2, sizeof(struct m0_idx));
  ptr_motrkvs_reader->idx_ctx->idx_count = 2;
  ptr_motrkvs_reader->idx_ctx->n_initialized_contexts = 1;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  ptr_motrkvs_reader->clean_up_contexts();
  EXPECT_EQ(nullptr, ptr_motrkvs_reader->reader_context);
  EXPECT_EQ(nullptr, ptr_motrkvs_reader->idx_ctx);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalTest) {
  S3CallBack s3motrkvscallbackobj;

  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));

  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalFailTest) {
  S3CallBack s3motrkvscallbackobj;

  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillRepeatedly(Return(-1));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _)).Times(0);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _)).Times(0);

  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalIdxPresentTest) {
  S3CallBack s3motrkvscallbackobj;

  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));

  ptr_motrkvs_reader->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  ptr_motrkvs_reader->idx_ctx->idx =
      (struct m0_idx *)calloc(2, sizeof(struct m0_idx));
  ptr_motrkvs_reader->idx_ctx->idx_count = 2;
  ptr_motrkvs_reader->idx_ctx->n_initialized_contexts = 1;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(2);

  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_EQ(1, ptr_motrkvs_reader->idx_ctx->idx_count);
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalTestEmpty) {
  S3CallBack s3motrkvscallbackobj;

  test_key = "";  // Empty key string

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));

  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalSuccessfulTest) {
  S3CallBack s3motrkvscallbackobj;
  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));

  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  ptr_motrkvs_reader->get_keyval_successful();
  EXPECT_TRUE(ptr_motrkvs_reader->get_state() ==
              S3MotrKVSReaderOpState::present);
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalFailedTest) {
  S3CallBack s3motrkvscallbackobj;
  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  ptr_motrkvs_reader->reader_context.reset(new S3MotrKVSReaderContext(
      ptr_mock_s3request, NULL, NULL, ptr_mock_s3motr));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  ptr_motrkvs_reader->get_keyval_failed();
  EXPECT_TRUE(ptr_motrkvs_reader->get_state() ==
              S3MotrKVSReaderOpState::failed);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
}

TEST_F(S3MotrKVSReaderTest, GetKeyvalFailedTestMissing) {
  S3CallBack s3motrkvscallbackobj;
  test_key = "utTestKey";

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail_enoent));
  ptr_motrkvs_reader->reader_context.reset(new S3MotrKVSReaderContext(
      ptr_mock_s3request, NULL, NULL, ptr_mock_s3motr));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-ENOENT));
  ptr_motrkvs_reader->get_keyval(
      index_oid, test_key,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  ptr_motrkvs_reader->get_keyval_failed();
  EXPECT_TRUE(ptr_motrkvs_reader->get_state() ==
              S3MotrKVSReaderOpState::missing);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
}

TEST_F(S3MotrKVSReaderTest, NextKeyvalTest) {
  S3CallBack s3motrkvscallbackobj;
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));
  ptr_motrkvs_reader->next_keyval(
      index_oid, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, NextKeyvalIdxPresentTest) {
  S3CallBack s3motrkvscallbackobj;
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));

  ptr_motrkvs_reader->idx_ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  ptr_motrkvs_reader->idx_ctx->idx =
      (struct m0_idx *)calloc(2, sizeof(struct m0_idx));
  ptr_motrkvs_reader->idx_ctx->idx_count = 2;
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);

  ptr_motrkvs_reader->next_keyval(
      index_oid, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_EQ(1, ptr_motrkvs_reader->idx_ctx->idx_count);
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, NextKeyvalSuccessfulTest) {
  S3CallBack s3motrkvscallbackobj;

  index_name = "ACCOUNT/s3ut_test";
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));

  ptr_motrkvs_reader->next_keyval(
      index_oid, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  EXPECT_TRUE(ptr_motrkvs_reader->get_state() ==
              S3MotrKVSReaderOpState::present);
  EXPECT_EQ(ptr_motrkvs_reader->last_result_keys_values.size(), 1);
  EXPECT_TRUE(s3motrkvscallbackobj.success_called);
  EXPECT_FALSE(s3motrkvscallbackobj.fail_called);
}

TEST_F(S3MotrKVSReaderTest, NextKeyvalFailedTest) {
  S3CallBack s3motrkvscallbackobj;
  index_name = "ACCOUNT/s3ut_test";
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));

  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  ptr_motrkvs_reader->reader_context.reset(new S3MotrKVSReaderContext(
      ptr_mock_s3request, NULL, NULL, ptr_mock_s3motr));
  ptr_motrkvs_reader->next_keyval(
      index_oid, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  ptr_motrkvs_reader->next_keyval_failed();
  EXPECT_TRUE(ptr_motrkvs_reader->get_state() ==
              S3MotrKVSReaderOpState::failed);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
}

TEST_F(S3MotrKVSReaderTest, NextKeyvalFailedTestMissing) {
  S3CallBack s3motrkvscallbackobj;
  index_name = "ACCOUNT/s3ut_test";
  test_key = "utTestKey";
  nr_kvp = 5;

  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_init(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_kvs_test_motr_idx_op));
  EXPECT_CALL(*ptr_mock_s3motr, motr_idx_fini(_)).Times(1);
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_setup(_, _, _));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail_enoent));
  EXPECT_CALL(*ptr_mock_s3motr, motr_op_rc(_)).WillRepeatedly(Return(-ENOENT));
  ptr_motrkvs_reader->next_keyval(
      index_oid, test_key, nr_kvp,
      std::bind(&S3CallBack::on_success, &s3motrkvscallbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrkvscallbackobj));

  ptr_motrkvs_reader->next_keyval_failed();
  EXPECT_TRUE(ptr_motrkvs_reader->get_state() ==
              S3MotrKVSReaderOpState::missing);
  EXPECT_TRUE(s3motrkvscallbackobj.fail_called);
  EXPECT_FALSE(s3motrkvscallbackobj.success_called);
}

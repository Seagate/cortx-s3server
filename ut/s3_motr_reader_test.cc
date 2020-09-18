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

#include "mock_s3_motr_wrapper.h"
#include "mock_s3_request_object.h"
#include "s3_callback_test_helpers.h"
#include "s3_motr_layout.h"
#include "s3_motr_reader.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

static int s3_test_motr_obj_op(struct m0_obj *obj, enum m0_obj_opcode opcode,
                               struct m0_indexvec *ext, struct m0_bufvec *data,
                               struct m0_bufvec *attr, uint64_t mask,
                               uint32_t flags, struct m0_op **op) {
  *op = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  return 0;
}

static int s3_test_allocate_op(struct m0_entity *entity, struct m0_op **ops) {
  *ops = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  return 0;
}

static void s3_test_free_motr_op(struct m0_op *op) { free(op); }

static void s3_test_motr_op_launch(uint64_t, struct m0_op **op, uint32_t nr,
                                   MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3MotrReaderContext *app_ctx =
      (S3MotrReaderContext *)ctx->application_context;
  struct s3_motr_op_context *op_ctx = app_ctx->get_motr_op_ctx();
  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_stable(test_motr_op);
    s3_test_free_motr_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

static void s3_dummy_motr_op_launch(uint64_t, struct m0_op **op, uint32_t nr,
                                    MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3MotrReaderContext *app_ctx =
      (S3MotrReaderContext *)ctx->application_context;
  struct s3_motr_op_context *op_ctx = app_ctx->get_motr_op_ctx();
  op_ctx->op_count = 0;
}

class S3MotrReaderTest : public testing::Test {
 protected:
  S3MotrReaderTest() {
    oid = {0x1ffff, 0x1ffff};
    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();

    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_motr_api_mock = std::make_shared<MockS3Motr>();
    motr_reader_ptr = std::make_shared<S3MotrReader>(
        request_mock, oid, layout_id, s3_motr_api_mock);
  }

  ~S3MotrReaderTest() { event_base_free(evbase); }

  struct m0_uint128 oid;
  int layout_id;
  evbase_t *evbase;
  evhtp_request_t *req;
  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<MockS3Motr> s3_motr_api_mock;
  std::shared_ptr<S3MotrReader> motr_reader_ptr;
};

TEST_F(S3MotrReaderTest, Constructor) {
  EXPECT_EQ(S3MotrReaderOpState::start, motr_reader_ptr->get_state());
  EXPECT_EQ(request_mock, motr_reader_ptr->request);
  EXPECT_EQ(0, motr_reader_ptr->iteration_index);
  EXPECT_EQ(0, motr_reader_ptr->last_index);
  EXPECT_FALSE(motr_reader_ptr->is_object_opened);
  EXPECT_TRUE(motr_reader_ptr->obj_ctx == nullptr);
}

TEST_F(S3MotrReaderTest, CleanupContexts) {
  motr_reader_ptr->obj_ctx = (struct s3_motr_obj_context *)calloc(
      1, sizeof(struct s3_motr_obj_context));
  motr_reader_ptr->obj_ctx->objs =
      (struct m0_obj *)calloc(2, sizeof(struct m0_obj));
  motr_reader_ptr->obj_ctx->obj_count = 2;
  motr_reader_ptr->obj_ctx->n_initialized_contexts = 2;
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(2);
  motr_reader_ptr->clean_up_contexts();
  EXPECT_TRUE(motr_reader_ptr->open_context == nullptr);
  EXPECT_TRUE(motr_reader_ptr->reader_context == nullptr);
  EXPECT_TRUE(motr_reader_ptr->obj_ctx == nullptr);
}

TEST_F(S3MotrReaderTest, OpenObjectDataTest) {
  S3CallBack s3motrreader_callbackobj;

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_op(_, _, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_obj_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  size_t num_of_blocks_to_read = 2;
  motr_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj));

  EXPECT_TRUE(s3motrreader_callbackobj.success_called);
  EXPECT_FALSE(s3motrreader_callbackobj.fail_called);
}

TEST_F(S3MotrReaderTest, OpenObjectCheckNoHoleFlagTest) {
  S3CallBack s3motrreader_callbackobj;

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock,
              motr_obj_op(_, _, _, _, _, _, M0_OOF_NOHOLE, _))
      .WillOnce(Invoke(s3_test_motr_obj_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  size_t num_of_blocks_to_read = 2;
  motr_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj));

  EXPECT_TRUE(s3motrreader_callbackobj.success_called);
  EXPECT_FALSE(s3motrreader_callbackobj.fail_called);
}

TEST_F(S3MotrReaderTest, ReadObjectDataTest) {
  S3CallBack s3motrreader_callbackobj;

  motr_reader_ptr->obj_ctx = (struct s3_motr_obj_context *)calloc(
      1, sizeof(struct s3_motr_obj_context));
  motr_reader_ptr->obj_ctx->objs =
      (struct m0_obj *)calloc(1, sizeof(struct m0_obj));
  motr_reader_ptr->obj_ctx->obj_count = 1;
  motr_reader_ptr->obj_ctx->n_initialized_contexts = 1;
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_op(_, _, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_obj_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  size_t num_of_blocks_to_read = 2;
  motr_reader_ptr->is_object_opened = true;
  motr_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj));

  EXPECT_TRUE(s3motrreader_callbackobj.success_called);
  EXPECT_FALSE(s3motrreader_callbackobj.fail_called);
}

TEST_F(S3MotrReaderTest, ReadObjectDataCheckNoHoleFlagTest) {
  S3CallBack s3motrreader_callbackobj;

  motr_reader_ptr->obj_ctx = (struct s3_motr_obj_context *)calloc(
      1, sizeof(struct s3_motr_obj_context));
  motr_reader_ptr->obj_ctx->objs =
      (struct m0_obj *)calloc(1, sizeof(struct m0_obj));
  motr_reader_ptr->obj_ctx->obj_count = 1;
  motr_reader_ptr->obj_ctx->n_initialized_contexts = 1;
  EXPECT_CALL(*s3_motr_api_mock,
              motr_obj_op(_, _, _, _, _, _, M0_OOF_NOHOLE, _))
      .WillOnce(Invoke(s3_test_motr_obj_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));

  size_t num_of_blocks_to_read = 2;
  motr_reader_ptr->is_object_opened = true;
  motr_reader_ptr->read_object_data(
      num_of_blocks_to_read,
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj));

  EXPECT_TRUE(s3motrreader_callbackobj.success_called);
  EXPECT_FALSE(s3motrreader_callbackobj.fail_called);
}

TEST_F(S3MotrReaderTest, ReadObjectDataSuccessful) {
  S3CallBack s3motrreader_callbackobj;
  motr_reader_ptr->reader_context.reset(
      new S3MotrReaderContext(request_mock, NULL, NULL, 1));

  motr_reader_ptr->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj);
  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj);
  ;

  motr_reader_ptr->read_object_successful();
  EXPECT_TRUE(motr_reader_ptr->get_state() == S3MotrReaderOpState::success);
  EXPECT_TRUE(s3motrreader_callbackobj.success_called);
  EXPECT_FALSE(s3motrreader_callbackobj.fail_called);
}

TEST_F(S3MotrReaderTest, ReadObjectDataFailed) {
  S3CallBack s3motrreader_callbackobj;

  motr_reader_ptr->reader_context.reset(
      new S3MotrReaderContext(request_mock, NULL, NULL, 1));

  motr_reader_ptr->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj);
  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj);
  ;
  motr_reader_ptr->reader_context->ops_response[0].error_code = -EACCES;

  motr_reader_ptr->read_object_failed();
  EXPECT_TRUE(motr_reader_ptr->get_state() == S3MotrReaderOpState::failed);
  EXPECT_TRUE(s3motrreader_callbackobj.fail_called);
  EXPECT_FALSE(s3motrreader_callbackobj.success_called);
}

TEST_F(S3MotrReaderTest, ReadObjectDataFailedMissing) {
  S3CallBack s3motrreader_callbackobj;

  motr_reader_ptr->reader_context.reset(
      new S3MotrReaderContext(request_mock, NULL, NULL, 1));

  motr_reader_ptr->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj);
  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj);
  ;
  motr_reader_ptr->reader_context->ops_response[0].error_code = -ENOENT;

  motr_reader_ptr->read_object_failed();
  EXPECT_TRUE(motr_reader_ptr->get_state() == S3MotrReaderOpState::missing);
  EXPECT_TRUE(s3motrreader_callbackobj.fail_called);
  EXPECT_FALSE(s3motrreader_callbackobj.success_called);
}

TEST_F(S3MotrReaderTest, OpenObjectTest) {
  motr_reader_ptr->num_of_blocks_to_read = 2;
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_dummy_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  S3MotrReader *p = motr_reader_ptr.get();
  motr_reader_ptr->open_object(
      std::bind(&S3MotrReader::open_object_successful, p),
      std::bind(&S3MotrReader::open_object_failed, p));
  EXPECT_FALSE(motr_reader_ptr->is_object_opened);
}

TEST_F(S3MotrReaderTest, OpenObjectFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_reader_ptr =
      std::make_shared<S3MotrReader>(request_mock, oid, 1, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_dummy_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj);

  S3MotrReader *p = motr_reader_ptr.get();

  motr_reader_ptr->open_object(
      std::bind(&S3MotrReader::open_object_successful, p),
      std::bind(&S3MotrReader::open_object_failed, p));
  motr_reader_ptr->open_object_failed();

  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
}

TEST_F(S3MotrReaderTest, OpenObjectMissingTest) {
  S3CallBack s3motrreader_callbackobj;

  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(-ENOENT));
  motr_reader_ptr->open_context.reset(
      new S3MotrReaderContext(request_mock, NULL, NULL, 1, s3_motr_api_mock));
  motr_reader_ptr->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj);
  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj);
  ;
  motr_reader_ptr->open_context->ops_response[0].error_code = -ENOENT;

  motr_reader_ptr->open_object_failed();
  EXPECT_TRUE(motr_reader_ptr->get_state() == S3MotrReaderOpState::missing);
  EXPECT_TRUE(s3motrreader_callbackobj.fail_called);
  EXPECT_FALSE(s3motrreader_callbackobj.success_called);
}

TEST_F(S3MotrReaderTest, OpenObjectErrFailedTest) {
  S3CallBack s3motrreader_callbackobj;

  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(-EACCES));
  motr_reader_ptr->open_context.reset(
      new S3MotrReaderContext(request_mock, NULL, NULL, 1, s3_motr_api_mock));
  motr_reader_ptr->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj);
  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj);
  ;
  motr_reader_ptr->open_context->ops_response[0].error_code = -EACCES;

  motr_reader_ptr->open_object_failed();
  EXPECT_TRUE(motr_reader_ptr->get_state() == S3MotrReaderOpState::failed);
  EXPECT_TRUE(s3motrreader_callbackobj.fail_called);
  EXPECT_FALSE(s3motrreader_callbackobj.success_called);
}

TEST_F(S3MotrReaderTest, OpenObjectSuccessTest) {
  S3CallBack s3motrreader_callbackobj;
  motr_reader_ptr->obj_ctx = (struct s3_motr_obj_context *)calloc(
      1, sizeof(struct s3_motr_obj_context));
  motr_reader_ptr->obj_ctx->objs =
      (struct m0_obj *)calloc(1, sizeof(struct m0_obj));
  motr_reader_ptr->obj_ctx->obj_count = 1;
  motr_reader_ptr->obj_ctx->n_initialized_contexts = 1;
  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(0));
  motr_reader_ptr->open_context.reset(
      new S3MotrReaderContext(request_mock, NULL, NULL, 1, s3_motr_api_mock));
  motr_reader_ptr->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3motrreader_callbackobj);
  motr_reader_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3motrreader_callbackobj);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_dummy_motr_op_launch));

  motr_reader_ptr->num_of_blocks_to_read = 2;
  motr_reader_ptr->open_object_successful();
  EXPECT_TRUE(motr_reader_ptr->is_object_opened);
}

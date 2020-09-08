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
#include "s3_async_buffer_opt.h"
#include "s3_callback_test_helpers.h"
#include "s3_motr_layout.h"
#include "s3_motr_writer.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

extern S3Option *g_option_instance;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

static int s3_test_motr_entity(struct m0_entity *entity, struct m0_op **op) {
  *op = (struct m0_op *)calloc(1, sizeof(struct m0_op));
  return 0;
}

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

  S3MotrWiterContext *app_ctx = (S3MotrWiterContext *)ctx->application_context;
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

  S3MotrWiterContext *app_ctx = (S3MotrWiterContext *)ctx->application_context;
  struct s3_motr_op_context *op_ctx = app_ctx->get_motr_op_ctx();
  op_ctx->op_count = 0;
}

static void s3_test_motr_op_launch_fail(uint64_t, struct m0_op **op,
                                        uint32_t nr, MotrOpType type) {
  struct s3_motr_context_obj *ctx =
      (struct s3_motr_context_obj *)op[0]->op_datum;

  S3MotrWiterContext *app_ctx = (S3MotrWiterContext *)ctx->application_context;
  struct s3_motr_op_context *op_ctx = app_ctx->get_motr_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_op *test_motr_op = op[i];
    s3_motr_op_failed(test_motr_op);
    s3_test_free_motr_op(test_motr_op);
  }
  op_ctx->op_count = 0;
}

class S3MotrWiterTest : public testing::Test {
 protected:
  S3MotrWiterTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_motr_api_mock = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*s3_motr_api_mock, m0_h_ufid_next(_)).WillRepeatedly(Return(0));
    buffer = std::make_shared<S3AsyncBufferOptContainer>(
        g_option_instance->get_libevent_pool_buffer_size());
    fourk_buffer.assign(g_option_instance->get_libevent_pool_buffer_size(),
                        'A');
    obj_oid = {0xffff, 0xffff};
    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();
  }

  ~S3MotrWiterTest() { event_base_free(evbase); }

  // Helper function
  evbuf_t *get_evbuf_t_with_data(std::string content) {
    evbuf_t *buf = evbuffer_new();
    // buf lifetime is managed by S3AsyncBufferContainer
    evbuffer_add(buf, content.c_str(), content.length());
    return buf;
  }

  // gets the internal data pointer with length from evbuf_t
  const char *get_datap_4_evbuf_t(evbuf_t *buf) {
    return (const char *)evbuffer_pullup(buf, evbuffer_get_length(buf));
  }

  evbase_t *evbase;
  evhtp_request_t *req;
  struct m0_uint128 obj_oid;
  int layout_id;
  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<MockS3Motr> s3_motr_api_mock;
  std::shared_ptr<S3MotrWiter> motr_writer_ptr;

  std::shared_ptr<S3AsyncBufferOptContainer> buffer;
  std::string fourk_buffer;
};

TEST_F(S3MotrWiterTest, Constructor) {
  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  EXPECT_EQ(S3MotrWiterOpState::start, motr_writer_ptr->get_state());
  EXPECT_EQ(request_mock, motr_writer_ptr->request);
  EXPECT_EQ(0, motr_writer_ptr->size_in_current_write);
  EXPECT_EQ(0, motr_writer_ptr->total_written);
  EXPECT_FALSE(motr_writer_ptr->is_object_opened);
  EXPECT_TRUE(motr_writer_ptr->obj_ctx == nullptr);
  EXPECT_TRUE(motr_writer_ptr->place_holder_for_last_unit == NULL);
}

TEST_F(S3MotrWiterTest, Constructor2) {
  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, 0, s3_motr_api_mock);
  EXPECT_EQ(S3MotrWiterOpState::start, motr_writer_ptr->get_state());
  EXPECT_EQ(request_mock, motr_writer_ptr->request);
  EXPECT_EQ(0, motr_writer_ptr->size_in_current_write);
  EXPECT_EQ(0, motr_writer_ptr->total_written);
  EXPECT_TRUE(motr_writer_ptr->place_holder_for_last_unit == NULL);
}

TEST_F(S3MotrWiterTest, CreateObjectTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_create(_, _))
      .WillOnce(Invoke(s3_test_motr_entity));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), layout_id);

  EXPECT_TRUE(S3MotrWiter_callbackobj.success_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, CreateObjectEntityCreateFailTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_create(_, _)).WillOnce(Return(-1));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), layout_id);

  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, CreateObjectSuccessfulTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_create(_, _))
      .WillOnce(Invoke(s3_test_motr_entity));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), layout_id);

  EXPECT_TRUE(S3MotrWiter_callbackobj.success_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, DeleteObjectSuccessfulTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_delete(_, _))
      .WillOnce(Invoke(s3_test_motr_entity));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->delete_object(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), layout_id);

  EXPECT_TRUE(S3MotrWiter_callbackobj.success_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, DeleteObjectFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));

  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->delete_object(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), layout_id);

  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, DeleteObjectmotrEntityDeleteFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;

  std::vector<struct m0_uint128> oids;
  std::vector<int> layoutids;
  for (int i = 0; i < 3; i++) {
    oids.push_back(obj_oid);
    layoutids.push_back(layout_id);
  }

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .Times(oids.size())
      .WillRepeatedly(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_delete(_, _))
      .Times(1)
      .WillRepeatedly(Return(-1));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(0));

  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->delete_objects(
      oids, layoutids,
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj));

  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, DeleteObjectsSuccessfulTest) {
  S3CallBack S3MotrWiter_callbackobj;

  std::vector<struct m0_uint128> oids;
  std::vector<int> layoutids;
  for (int i = 0; i < 3; i++) {
    oids.push_back(obj_oid);
    layoutids.push_back(layout_id);
  }

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .Times(oids.size())
      .WillRepeatedly(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_delete(_, _))
      .Times(oids.size())
      .WillRepeatedly(Invoke(s3_test_motr_entity));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(2 * oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(0));

  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->delete_objects(
      oids, layoutids,
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj));

  EXPECT_TRUE(S3MotrWiter_callbackobj.success_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, DeleteObjectsFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;

  std::vector<struct m0_uint128> oids;
  std::vector<int> layoutids;
  for (int i = 0; i < 3; i++) {
    oids.push_back(obj_oid);
    layoutids.push_back(layout_id);
  }

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .Times(oids.size())
      .WillRepeatedly(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(oids.size());
  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  S3Option::get_instance()->set_eventbase(evbase);

  motr_writer_ptr->delete_objects(
      oids, layoutids,
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj));

  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, OpenObjectsTest) {
  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_dummy_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  motr_writer_ptr->open_objects();

  EXPECT_FALSE(motr_writer_ptr->is_object_opened);
}

TEST_F(S3MotrWiterTest, OpenObjectsEntityOpenFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;
  int rc;
  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillRepeatedly(Return(-E2BIG));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(0);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _)).Times(0);

  motr_writer_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj);
  rc = motr_writer_ptr->open_objects();
  EXPECT_EQ(-E2BIG, rc);
  EXPECT_EQ(S3MotrWiterOpState::failed_to_launch, motr_writer_ptr->state);
  EXPECT_FALSE(motr_writer_ptr->is_object_opened);
}

TEST_F(S3MotrWiterTest, OpenObjectsFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_dummy_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  motr_writer_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj);

  motr_writer_ptr->open_objects();
  motr_writer_ptr->open_objects_failed();

  EXPECT_EQ(S3MotrWiterOpState::failed, motr_writer_ptr->state);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
}

TEST_F(S3MotrWiterTest, OpenObjectsFailedMissingTest) {
  S3CallBack S3MotrWiter_callbackobj;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_dummy_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  motr_writer_ptr->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj);

  motr_writer_ptr->open_objects();
  motr_writer_ptr->open_context->set_op_errno_for(0, -ENOENT);
  motr_writer_ptr->open_objects_failed();

  EXPECT_EQ(S3MotrWiterOpState::missing, motr_writer_ptr->state);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
}

TEST_F(S3MotrWiterTest, WriteContentSuccessfulTest) {
  S3CallBack S3MotrWiter_callbackobj;
  bool is_last_buf = true;
  std::string sdata("Hello.World");

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_op(_, _, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_motr_obj_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(2);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillRepeatedly(Invoke(s3_test_motr_op_launch));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);

  S3Option::get_instance()->set_eventbase(evbase);

  buffer->add_content(get_evbuf_t_with_data(fourk_buffer), false, false, true);
  buffer->add_content(get_evbuf_t_with_data(sdata.c_str()), false, is_last_buf,
                      true);
  buffer->freeze();
  motr_writer_ptr->write_content(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), buffer);

  EXPECT_TRUE(motr_writer_ptr->get_state() == S3MotrWiterOpState::saved);
  EXPECT_EQ(motr_writer_ptr->size_in_current_write,
            fourk_buffer.length() + sdata.length());
  EXPECT_TRUE(S3MotrWiter_callbackobj.success_called);
  EXPECT_FALSE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, WriteContentFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;
  bool is_last_buf = true;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillOnce(Invoke(s3_test_allocate_op));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _))
      .WillOnce(Invoke(s3_test_motr_op_launch_fail));
  EXPECT_CALL(*s3_motr_api_mock, motr_obj_fini(_)).Times(1);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_rc(_)).WillRepeatedly(Return(-EPERM));
  S3Option::get_instance()->set_eventbase(evbase);

  buffer->add_content(get_evbuf_t_with_data(fourk_buffer), is_last_buf);
  buffer->freeze();

  motr_writer_ptr->write_content(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), buffer);

  EXPECT_TRUE(motr_writer_ptr->get_state() == S3MotrWiterOpState::failed);
  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
}

TEST_F(S3MotrWiterTest, WriteEntityFailedTest) {
  S3CallBack S3MotrWiter_callbackobj;
  bool is_last_buf = true;

  motr_writer_ptr =
      std::make_shared<S3MotrWiter>(request_mock, obj_oid, 0, s3_motr_api_mock);
  motr_writer_ptr->set_layout_id(layout_id);

  EXPECT_CALL(*s3_motr_api_mock, motr_obj_init(_, _, _, _));
  EXPECT_CALL(*s3_motr_api_mock, motr_entity_open(_, _))
      .WillRepeatedly(Return(-E2BIG));
  EXPECT_CALL(*s3_motr_api_mock, motr_op_setup(_, _, _)).Times(0);
  EXPECT_CALL(*s3_motr_api_mock, motr_op_launch(_, _, _, _)).Times(0);
  S3Option::get_instance()->set_eventbase(evbase);

  buffer->add_content(get_evbuf_t_with_data(fourk_buffer), is_last_buf);
  buffer->freeze();

  motr_writer_ptr->write_content(
      std::bind(&S3CallBack::on_success, &S3MotrWiter_callbackobj),
      std::bind(&S3CallBack::on_failed, &S3MotrWiter_callbackobj), buffer);

  EXPECT_TRUE(motr_writer_ptr->get_state() ==
              S3MotrWiterOpState::failed_to_launch);
  EXPECT_FALSE(S3MotrWiter_callbackobj.success_called);
  EXPECT_TRUE(S3MotrWiter_callbackobj.fail_called);
}

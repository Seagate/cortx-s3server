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
 * Original creation date: 22-Feb-2017
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_request_object.h"
#include "s3_async_buffer_opt.h"
#include "s3_callback_test_helpers.h"
#include "s3_clovis_writer.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

extern S3Option *g_option_instance;

static void dummy_request_cb(evhtp_request_t *req, void *arg) {}

static int s3_test_clovis_entity(struct m0_clovis_entity *entity,
                                 struct m0_clovis_op **op) {
  *op = (struct m0_clovis_op *)calloc(1, sizeof(struct m0_clovis_op));
  return 0;
}

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

  S3ClovisWriterContext *app_ctx =
      (S3ClovisWriterContext *)ctx->application_context;
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

  S3ClovisWriterContext *app_ctx =
      (S3ClovisWriterContext *)ctx->application_context;
  struct s3_clovis_op_context *op_ctx = app_ctx->get_clovis_op_ctx();

  for (int i = 0; i < (int)nr; i++) {
    struct m0_clovis_op *test_clovis_op = op[i];
    test_clovis_op->op_sm.sm_rc = -EPERM;
    s3_clovis_op_failed(test_clovis_op);
    s3_test_free_clovis_op(test_clovis_op);
  }
  op_ctx->op_count = 0;
}

class S3ClovisWriterTest : public testing::Test {
 protected:
  S3ClovisWriterTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_clovis_api_mock = std::make_shared<MockS3Clovis>();
    buffer = std::make_shared<S3AsyncBufferOptContainer>(
        g_option_instance->get_libevent_pool_buffer_size());
    fourk_buffer.assign(g_option_instance->get_libevent_pool_buffer_size(),
                        'A');
    obj_oid = {0xffff, 0xffff};
  }

  ~S3ClovisWriterTest() { event_base_free(evbase); }

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
  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<MockS3Clovis> s3_clovis_api_mock;
  std::shared_ptr<S3ClovisWriter> clovis_writer_ptr;

  std::shared_ptr<S3AsyncBufferOptContainer> buffer;
  std::string fourk_buffer;
};

TEST_F(S3ClovisWriterTest, Constructor) {
  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  EXPECT_EQ(S3ClovisWriterOpState::start, clovis_writer_ptr->get_state());
  EXPECT_EQ(request_mock, clovis_writer_ptr->request);
  EXPECT_EQ(0, clovis_writer_ptr->size_in_current_write);
  EXPECT_EQ(0, clovis_writer_ptr->total_written);
  EXPECT_EQ(0, clovis_writer_ptr->ops_count);
  EXPECT_TRUE(clovis_writer_ptr->place_holder_for_last_unit != NULL);
}

TEST_F(S3ClovisWriterTest, Constructor2) {
  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock);
  EXPECT_EQ(S3ClovisWriterOpState::start, clovis_writer_ptr->get_state());
  EXPECT_EQ(request_mock, clovis_writer_ptr->request);
  EXPECT_EQ(0, clovis_writer_ptr->size_in_current_write);
  EXPECT_EQ(0, clovis_writer_ptr->total_written);
  EXPECT_EQ(0, clovis_writer_ptr->ops_count);
  EXPECT_TRUE(clovis_writer_ptr->place_holder_for_last_unit != NULL);
}

TEST_F(S3ClovisWriterTest, CreateObjectTest) {
  S3CallBack s3cloviswriter_callbackobj;

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_clovis_entity));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  S3Option::get_instance()->set_eventbase(evbase);

  clovis_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj));

  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == FALSE);
}

TEST_F(S3ClovisWriterTest, CreateObjectSuccessfulTest) {
  S3CallBack s3cloviswriter_callbackobj;

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_clovis_entity));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  S3Option::get_instance()->set_eventbase(evbase);

  clovis_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj));

  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == FALSE);
}

TEST_F(S3ClovisWriterTest, DeleteObjectSuccessfulTest) {
  S3CallBack s3cloviswriter_callbackobj;

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_clovis_entity));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  S3Option::get_instance()->set_eventbase(evbase);

  clovis_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj));

  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == FALSE);
}

TEST_F(S3ClovisWriterTest, DeleteObjectFailedTest) {
  S3CallBack s3cloviswriter_callbackobj;

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_entity_create(_, _))
      .WillOnce(Invoke(s3_test_clovis_entity));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));
  S3Option::get_instance()->set_eventbase(evbase);

  clovis_writer_ptr->create_object(
      std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj));

  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == TRUE);
}

TEST_F(S3ClovisWriterTest, DeleteObjectsSuccessfulTest) {
  S3CallBack s3cloviswriter_callbackobj;

  std::vector<struct m0_uint128> oids;
  for (int i = 0; i < 3; i++) oids.push_back(obj_oid);

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_clovis_api_mock, clovis_entity_delete(_, _))
      .Times(oids.size())
      .WillRepeatedly(Invoke(s3_test_clovis_entity));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  S3Option::get_instance()->set_eventbase(evbase);

  clovis_writer_ptr->delete_objects(
      oids, std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj));

  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == FALSE);
}

TEST_F(S3ClovisWriterTest, DeleteObjectsFailedTest) {
  S3CallBack s3cloviswriter_callbackobj;

  std::vector<struct m0_uint128> oids;
  for (int i = 0; i < 3; i++) oids.push_back(obj_oid);

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_clovis_api_mock, clovis_entity_delete(_, _))
      .Times(oids.size())
      .WillRepeatedly(Invoke(s3_test_clovis_entity));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _)).Times(oids.size());
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));
  S3Option::get_instance()->set_eventbase(evbase);

  clovis_writer_ptr->delete_objects(
      oids, std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj));

  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == TRUE);
}

TEST_F(S3ClovisWriterTest, WriteContentSuccessfulTest) {
  S3CallBack s3cloviswriter_callbackobj;
  bool is_last_buf = true;
  std::string sdata("Hello.World");

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch));
  S3Option::get_instance()->set_eventbase(evbase);

  buffer->add_content(get_evbuf_t_with_data(fourk_buffer));
  buffer->add_content(get_evbuf_t_with_data(sdata.c_str()), is_last_buf);
  buffer->freeze();

  clovis_writer_ptr->write_content(
      std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj), buffer);

  EXPECT_TRUE(clovis_writer_ptr->get_state() == S3ClovisWriterOpState::saved);
  EXPECT_EQ(clovis_writer_ptr->size_in_current_write,
            fourk_buffer.length() + sdata.length());
  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == TRUE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == FALSE);
}

TEST_F(S3ClovisWriterTest, WriteContentFailedTest) {
  S3CallBack s3cloviswriter_callbackobj;
  bool is_last_buf = true;

  clovis_writer_ptr = std::make_shared<S3ClovisWriter>(request_mock, obj_oid);
  clovis_writer_ptr->reset_clovis_api(s3_clovis_api_mock);

  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_init(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_obj_op(_, _, _, _, _, _, _))
      .WillOnce(Invoke(s3_test_clovis_obj_op));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_setup(_, _, _));
  EXPECT_CALL(*s3_clovis_api_mock, clovis_op_launch(_, _, _))
      .WillOnce(Invoke(s3_test_clovis_op_launch_fail));
  S3Option::get_instance()->set_eventbase(evbase);

  buffer->add_content(get_evbuf_t_with_data(fourk_buffer), is_last_buf);
  buffer->freeze();

  clovis_writer_ptr->write_content(
      std::bind(&S3CallBack::on_success, &s3cloviswriter_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3cloviswriter_callbackobj), buffer);

  EXPECT_TRUE(clovis_writer_ptr->get_state() == S3ClovisWriterOpState::failed);
  EXPECT_TRUE(s3cloviswriter_callbackobj.success_called == FALSE);
  EXPECT_TRUE(s3cloviswriter_callbackobj.fail_called == TRUE);
}

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

class S3ClovisKvsWriterTest : public testing::Test {
 protected:
  S3ClovisKvsWriterTest() {
    evbase = event_base_new();
    req = evhtp_request_new(dummy_request_cb, evbase);
    EvhtpWrapper *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3clovis = std::make_shared<MockS3Clovis>();
    EXPECT_CALL(*ptr_mock_s3clovis, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    EXPECT_CALL(*ptr_mock_s3clovis, clovis_op_rc(_)).WillRepeatedly(Return(0));

    action_under_test = std::make_shared<S3ClovisKVSWriter>(ptr_mock_request,
                                                            ptr_mock_s3clovis);
    oid = {0xffff, 0xfff1f};
  }

  ~S3ClovisKvsWriterTest() { event_base_free(evbase); }

  evbase_t *evbase;
  evhtp_request_t *req;
  struct m0_uint128 oid;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3clovis;
  std::shared_ptr<S3ClovisKVSWriter> action_under_test;
  S3ClovisKVSWriter *p_cloviskvs;
};

TEST_F(S3ClovisKvsWriterTest, PutKeyValEmpty) {
  S3CallBack s3cloviskvscallbackobj;

  ASSERT_DEATH((action_under_test->put_keyval(
                   oid, "", "",
                   std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
                   std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj))),
               ".*");
}

TEST_F(S3ClovisKvsWriterTest, DelKeyValEmpty) {
  S3CallBack s3cloviskvscallbackobj;

  ASSERT_DEATH(
      (action_under_test->delete_keyval(
          oid, "", std::bind(&S3CallBack::on_success, &s3cloviskvscallbackobj),
          std::bind(&S3CallBack::on_failed, &s3cloviskvscallbackobj))),
      ".*");
}

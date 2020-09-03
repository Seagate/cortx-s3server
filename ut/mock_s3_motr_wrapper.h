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

#pragma once

#ifndef __S3_UT_MOCK_S3_MOTR_WRAPPER_H__
#define __S3_UT_MOCK_S3_MOTR_WRAPPER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <functional>
#include <iostream>
#include "motr_helpers.h"
#include "s3_motr_rw_common.h"
#include "s3_motr_wrapper.h"

class MockS3Motr : public MotrAPI {
 public:
  MockS3Motr() : MotrAPI() {}
  MOCK_METHOD3(motr_idx_init, void(struct m0_idx *idx, struct m0_realm *parent,
                                   const struct m0_uint128 *id));
  MOCK_METHOD1(motr_idx_fini, void(struct m0_idx *idx));
  MOCK_METHOD4(motr_obj_init, void(struct m0_obj *obj, struct m0_realm *parent,
                                   const struct m0_uint128 *id, int layout_id));
  MOCK_METHOD1(motr_obj_fini, void(struct m0_obj *obj));
  MOCK_METHOD2(motr_entity_open,
               int(struct m0_entity *entity, struct m0_op **op));
  MOCK_METHOD2(motr_entity_create,
               int(struct m0_entity *entity, struct m0_op **op));
  MOCK_METHOD2(motr_entity_delete,
               int(struct m0_entity *entity, struct m0_op **op));
  MOCK_METHOD3(motr_op_setup,
               void(struct m0_op *op, const struct m0_op_ops *ops,
                    m0_time_t linger));
  MOCK_METHOD7(motr_idx_op,
               int(struct m0_idx *idx, enum m0_idx_opcode opcode,
                   struct m0_bufvec *keys, struct m0_bufvec *vals, int *rcs,
                   unsigned int flags, struct m0_op **op));
  MOCK_METHOD8(motr_obj_op, int(struct m0_obj *obj, enum m0_obj_opcode opcode,
                                struct m0_indexvec *ext, struct m0_bufvec *data,
                                struct m0_bufvec *attr, uint64_t mask,
                                uint32_t flags, struct m0_op **op));
  MOCK_METHOD4(motr_op_launch,
               void(uint64_t, struct m0_op **, uint32_t, MotrOpType));
  MOCK_METHOD3(motr_op_wait,
               int(struct m0_op *op, uint64_t bits, m0_time_t to));
  MOCK_METHOD1(motr_sync_op_init, int(struct m0_op **sync_op));
  MOCK_METHOD2(motr_sync_entity_add,
               int(struct m0_op *sync_op, struct m0_entity *entity));
  MOCK_METHOD2(motr_sync_op_add, int(struct m0_op *sync_op, struct m0_op *op));
  MOCK_METHOD1(motr_op_rc, int(const struct m0_op *op));
  MOCK_METHOD1(m0_h_ufid_next, int(struct m0_uint128 *ufid));
};
#endif

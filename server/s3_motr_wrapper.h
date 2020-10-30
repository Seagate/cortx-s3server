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

#ifndef __S3_SERVER_S3_MOTR_WRAPPER_H__
#define __S3_SERVER_S3_MOTR_WRAPPER_H__

#include <functional>
#include <iostream>

#include "motr_helpers.h"
#include "s3_option.h"
#include "s3_fi_common.h"
#include "s3_post_to_main_loop.h"

enum class MotrOpType {
  unknown,
  openobj,
  createobj,
  writeobj,
  readobj,
  deleteobj,
  createidx,
  deleteidx,
  headidx,
  getkv,
  putkv,
  deletekv,
};

class MotrAPI {
 public:
  virtual void motr_idx_init(struct m0_idx *idx, struct m0_realm *parent,
                             const struct m0_uint128 *id) = 0;

  virtual void motr_idx_fini(struct m0_idx *idx) = 0;

  virtual int motr_sync_op_init(struct m0_op **sync_op) = 0;

  virtual int motr_sync_entity_add(struct m0_op *sync_op,
                                   struct m0_entity *entity) = 0;

  virtual int motr_sync_op_add(struct m0_op *sync_op, struct m0_op *op) = 0;

  virtual void motr_obj_init(struct m0_obj *obj, struct m0_realm *parent,
                             const struct m0_uint128 *id, int layout_id) = 0;

  virtual void motr_obj_fini(struct m0_obj *obj) = 0;

  virtual int motr_entity_open(struct m0_entity *entity, struct m0_op **op) = 0;

  virtual int motr_entity_create(struct m0_entity *entity,
                                 struct m0_op **op) = 0;

  virtual int motr_entity_delete(struct m0_entity *entity,
                                 struct m0_op **op) = 0;

  virtual void motr_op_setup(struct m0_op *op, const struct m0_op_ops *ops,
                             m0_time_t linger) = 0;

  virtual int motr_idx_op(struct m0_idx *idx, enum m0_idx_opcode opcode,
                          struct m0_bufvec *keys, struct m0_bufvec *vals,
                          int *rcs, unsigned int flags, struct m0_op **op) = 0;

  virtual int motr_obj_op(struct m0_obj *obj, enum m0_obj_opcode opcode,
                          struct m0_indexvec *ext, struct m0_bufvec *data,
                          struct m0_bufvec *attr, uint64_t mask, uint32_t flags,
                          struct m0_op **op) = 0;

  virtual void motr_op_launch(uint64_t addb_request_id, struct m0_op **op,
                              uint32_t nr,
                              MotrOpType type = MotrOpType::unknown) = 0;
  virtual int motr_op_wait(m0_op *op, uint64_t bits, m0_time_t to) = 0;

  virtual int motr_op_rc(const struct m0_op *op) = 0;
  virtual int m0_h_ufid_next(struct m0_uint128 *ufid) = 0;
};

class ConcreteMotrAPI : public MotrAPI {
 private:
  // xxx This currently assumes only one fake operation is invoked.
  void motr_fake_op_launch(struct m0_op **op, uint32_t nr);

  void motr_fake_redis_op_launch(struct m0_op **op, uint32_t nr);

  void motr_fi_op_launch(struct m0_op **op, uint32_t nr);

  bool is_motr_sync_should_be_faked();

  static void motr_op_launch_addb_add(uint64_t addb_request_id,
                                      struct m0_op **op, uint32_t nr);

 public:
  virtual void motr_idx_init(struct m0_idx *idx, struct m0_realm *parent,
                             const struct m0_uint128 *id);

  virtual void motr_obj_init(struct m0_obj *obj, struct m0_realm *parent,
                             const struct m0_uint128 *id, int layout_id);

  virtual void motr_obj_fini(struct m0_obj *obj);

  virtual int motr_sync_op_init(struct m0_op **sync_op);

  virtual int motr_sync_entity_add(struct m0_op *sync_op,
                                   struct m0_entity *entity);

  virtual int motr_sync_op_add(struct m0_op *sync_op, struct m0_op *op);

  virtual int motr_entity_open(struct m0_entity *entity, struct m0_op **op);

  virtual int motr_entity_create(struct m0_entity *entity, struct m0_op **op);

  virtual int motr_entity_delete(struct m0_entity *entity, struct m0_op **op);

  virtual void motr_op_setup(struct m0_op *op, const struct m0_op_ops *ops,
                             m0_time_t linger);

  virtual int motr_idx_op(struct m0_idx *idx, enum m0_idx_opcode opcode,
                          struct m0_bufvec *keys, struct m0_bufvec *vals,
                          int *rcs, unsigned int flags, struct m0_op **op);

  virtual void motr_idx_fini(struct m0_idx *idx);

  virtual int motr_obj_op(struct m0_obj *obj, enum m0_obj_opcode opcode,
                          struct m0_indexvec *ext, struct m0_bufvec *data,
                          struct m0_bufvec *attr, uint64_t mask, uint32_t flags,
                          struct m0_op **op);

  bool is_kvs_op(MotrOpType type);

  bool is_redis_kvs_op(S3Option *opts, MotrOpType type);

  virtual void motr_op_launch(uint64_t addb_request_id, struct m0_op **op,
                              uint32_t nr,
                              MotrOpType type = MotrOpType::unknown);

  // Used for sync motr calls
  virtual int motr_op_wait(m0_op *op, uint64_t bits, m0_time_t op_wait_period);

  virtual int motr_op_rc(const struct m0_op *op);

  virtual int m0_h_ufid_next(struct m0_uint128 *ufid);
};
#endif

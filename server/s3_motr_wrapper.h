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

#include "s3_motr_rw_common.h"
#include "s3_post_to_main_loop.h"

#include "motr_helpers.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_fake_motr_redis_kvs.h"
#include "s3_addb.h"

extern struct m0_ufid_generator s3_ufid_generator;

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
  virtual void clovis_idx_init(struct m0_idx *idx, struct m0_realm *parent,
                               const struct m0_uint128 *id) = 0;

  virtual void motr_idx_fini(struct m0_idx *idx) = 0;

  virtual int clovis_sync_op_init(struct m0_op **sync_op) = 0;

  virtual int clovis_sync_entity_add(struct m0_op *sync_op,
                                     struct m0_entity *entity) = 0;

  virtual int clovis_sync_op_add(struct m0_op *sync_op, struct m0_op *op) = 0;

  virtual void clovis_obj_init(struct m0_obj *obj, struct m0_realm *parent,
                               const struct m0_uint128 *id, int layout_id) = 0;

  virtual void motr_obj_fini(struct m0_obj *obj) = 0;

  virtual int clovis_entity_open(struct m0_entity *entity,
                                 struct m0_op **op) = 0;

  virtual int clovis_entity_create(struct m0_entity *entity,
                                   struct m0_op **op) = 0;

  virtual int clovis_entity_delete(struct m0_entity *entity,
                                   struct m0_op **op) = 0;

  virtual void clovis_op_setup(struct m0_op *op, const struct m0_op_ops *ops,
                               m0_time_t linger) = 0;

  virtual int clovis_idx_op(struct m0_idx *idx, enum m0_idx_opcode opcode,
                            struct m0_bufvec *keys, struct m0_bufvec *vals,
                            int *rcs, unsigned int flags,
                            struct m0_op **op) = 0;

  virtual int clovis_obj_op(struct m0_obj *obj, enum m0_obj_opcode opcode,
                            struct m0_indexvec *ext, struct m0_bufvec *data,
                            struct m0_bufvec *attr, uint64_t mask,
                            struct m0_op **op) = 0;

  virtual void clovis_op_launch(uint64_t addb_request_id, struct m0_op **op,
                                uint32_t nr,
                                MotrOpType type = MotrOpType::unknown) = 0;
  virtual int clovis_op_wait(m0_op *op, uint64_t bits, m0_time_t to) = 0;

  virtual int clovis_op_rc(const struct m0_op *op) = 0;
  virtual int m0_h_ufid_next(struct m0_uint128 *ufid) = 0;
};

class ConcreteMotrAPI : public MotrAPI {
 private:
  // xxx This currently assumes only one fake operation is invoked.
  void clovis_fake_op_launch(struct m0_op **op, uint32_t nr) {
    s3_log(S3_LOG_DEBUG, "", "Called\n");
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(
        1, sizeof(struct user_event_context));
    user_ctx->app_ctx = op[0];

    S3PostToMainLoop((void *)user_ctx)(s3_motr_dummy_op_stable);
  }

  void clovis_fake_redis_op_launch(struct m0_op **op, uint32_t nr) {
    s3_log(S3_LOG_DEBUG, "", "Entering\n");
    auto redis_ctx = S3FakeMotrRedisKvs::instance();

    for (uint32_t i = 0; i < nr; ++i) {
      struct m0_op *cop = op[i];

      assert(cop);

      if (cop->op_code == M0_IC_GET) {
        redis_ctx->kv_read(cop);
      } else if (M0_IC_NEXT == cop->op_code) {
        redis_ctx->kv_next(cop);
      } else if (M0_IC_PUT == cop->op_code) {
        redis_ctx->kv_write(cop);
      } else if (M0_IC_DEL == cop->op_code) {
        redis_ctx->kv_del(cop);
      } else {
        s3_log(S3_LOG_DEBUG, "", "Not a kvs op (%d) - ignore", cop->op_code);
        cop->op_rc = 0;
        s3_motr_op_stable(cop);
      }
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  }

  void clovis_fi_op_launch(struct m0_op **op, uint32_t nr) {
    s3_log(S3_LOG_DEBUG, "", "Called\n");
    for (uint32_t i = 0; i < nr; ++i) {
      struct user_event_context *user_ctx = (struct user_event_context *)calloc(
          1, sizeof(struct user_event_context));
      user_ctx->app_ctx = op[i];

      S3PostToMainLoop((void *)user_ctx)(s3_motr_dummy_op_failed);
    }
  }

  bool is_clovis_sync_should_be_faked() {
    auto inst = S3Option::get_instance();
    return inst->is_fake_clovis_putkv() || inst->is_fake_clovis_redis_kvs();
  }

  static void clovis_op_launch_addb_add(uint64_t addb_request_id,
                                        struct m0_op **op, uint32_t nr) {
    for (uint32_t i = 0; i < nr; ++i) {
      s3_log(S3_LOG_DEBUG, "", "request-to-clovis: request_id %" PRId64
                               ", clovis id %" PRId64 "\n",
             addb_request_id, op[i]->op_sm.sm_id);
      ADDB(S3_ADDB_REQUEST_TO_CLOVIS_ID, addb_request_id, op[i]->op_sm.sm_id);
    }
  }

 public:
  void clovis_idx_init(struct m0_idx *idx, struct m0_realm *parent,
                       const struct m0_uint128 *id) {
    m0_idx_init(idx, parent, id);
  }

  void clovis_obj_init(struct m0_obj *obj, struct m0_realm *parent,
                       const struct m0_uint128 *id, int layout_id) {
    m0_obj_init(obj, parent, id, layout_id);
  }

  void motr_obj_fini(struct m0_obj *obj) { m0_obj_fini(obj); }

  int clovis_sync_op_init(struct m0_op **sync_op) {
    if (s3_fi_is_enabled("clovis_sync_op_init_fail")) {
      return -1;
    } else {
      return m0_sync_op_init(sync_op);
    }
  }

  int clovis_sync_entity_add(struct m0_op *sync_op, struct m0_entity *entity) {
    if (is_clovis_sync_should_be_faked()) {
      return 0;
    }
    return m0_sync_entity_add(sync_op, entity);
  }

  int clovis_sync_op_add(struct m0_op *sync_op, struct m0_op *op) {
    if (is_clovis_sync_should_be_faked()) {
      return 0;
    }
    return m0_sync_op_add(sync_op, op);
  }

  int clovis_entity_open(struct m0_entity *entity, struct m0_op **op) {
    if (s3_fi_is_enabled("clovis_entity_open_fail")) {
      return -1;
    } else {
      return m0_entity_open(entity, op);
    }
  }

  int clovis_entity_create(struct m0_entity *entity, struct m0_op **op) {
    if (s3_fi_is_enabled("clovis_entity_create_fail")) {
      return -1;
    } else {
      return m0_entity_create(NULL, entity, op);
    }
  }

  int clovis_entity_delete(struct m0_entity *entity, struct m0_op **op) {
    if (s3_fi_is_enabled("clovis_entity_delete_fail")) {
      return -1;
    } else {
      return m0_entity_delete(entity, op);
    }
  }

  void clovis_op_setup(struct m0_op *op, const struct m0_op_ops *ops,
                       m0_time_t linger) {
    m0_op_setup(op, ops, linger);
  }

  int clovis_idx_op(struct m0_idx *idx, enum m0_idx_opcode opcode,
                    struct m0_bufvec *keys, struct m0_bufvec *vals, int *rcs,
                    unsigned int flags, struct m0_op **op) {
    if (s3_fi_is_enabled("clovis_idx_op_fail")) {
      return -1;
    } else {
      return m0_idx_op(idx, opcode, keys, vals, rcs, flags, op);
    }
  }

  void motr_idx_fini(struct m0_idx *idx) { m0_idx_fini(idx); }

  int clovis_obj_op(struct m0_obj *obj, enum m0_obj_opcode opcode,
                    struct m0_indexvec *ext, struct m0_bufvec *data,
                    struct m0_bufvec *attr, uint64_t mask, struct m0_op **op) {
    return m0_obj_op(obj, opcode, ext, data, attr, mask, op);
  }

  bool is_kvs_op(MotrOpType type) {
    return type == MotrOpType::getkv || type == MotrOpType::putkv ||
           type == MotrOpType::deletekv;
  }

  bool is_redis_kvs_op(S3Option *opts, MotrOpType type) {
    return opts && opts->is_fake_clovis_redis_kvs() && is_kvs_op(type);
  }

  void clovis_op_launch(uint64_t addb_request_id, struct m0_op **op,
                        uint32_t nr, MotrOpType type = MotrOpType::unknown) {
    S3Option *config = S3Option::get_instance();
    clovis_op_launch_addb_add(addb_request_id, op, nr);
    if ((config->is_fake_clovis_createobj() && type == MotrOpType::createobj) ||
        (config->is_fake_clovis_writeobj() && type == MotrOpType::writeobj) ||
        (config->is_fake_clovis_readobj() && type == MotrOpType::readobj) ||
        (config->is_fake_clovis_deleteobj() && type == MotrOpType::deleteobj) ||
        (config->is_fake_clovis_createidx() && type == MotrOpType::createidx) ||
        (config->is_fake_clovis_deleteidx() && type == MotrOpType::deleteidx) ||
        (config->is_fake_clovis_getkv() && type == MotrOpType::getkv) ||
        (config->is_fake_clovis_putkv() && type == MotrOpType::putkv) ||
        (config->is_fake_clovis_deletekv() && type == MotrOpType::deletekv)) {
      clovis_fake_op_launch(op, nr);
    } else if (is_redis_kvs_op(config, type)) {
      clovis_fake_redis_op_launch(op, nr);
    } else if ((type == MotrOpType::createobj &&
                s3_fi_is_enabled("clovis_obj_create_fail")) ||
               (type == MotrOpType::openobj &&
                s3_fi_is_enabled("clovis_obj_open_fail")) ||
               (type == MotrOpType::writeobj &&
                s3_fi_is_enabled("clovis_obj_write_fail")) ||
               (type == MotrOpType::deleteobj &&
                s3_fi_is_enabled("clovis_obj_delete_fail")) ||
               (type == MotrOpType::createidx &&
                s3_fi_is_enabled("clovis_idx_create_fail")) ||
               (type == MotrOpType::deleteidx &&
                s3_fi_is_enabled("clovis_idx_delete_fail")) ||
               (type == MotrOpType::deletekv &&
                s3_fi_is_enabled("clovis_kv_delete_fail")) ||
               (type == MotrOpType::putkv &&
                s3_fi_is_enabled("clovis_kv_put_fail")) ||
               (type == MotrOpType::getkv &&
                s3_fi_is_enabled("clovis_kv_get_fail"))) {
      clovis_fi_op_launch(op, nr);
    } else {
      s3_log(S3_LOG_DEBUG, "", "m0_op_launch will be used");
      m0_op_launch(op, nr);
    }
  }

  // Used for sync clovis calls
  int clovis_op_wait(m0_op *op, uint64_t bits, m0_time_t op_wait_period) {
    return m0_op_wait(op, bits, op_wait_period);
  }

  int clovis_op_rc(const struct m0_op *op) { return m0_rc(op); }
  int m0_h_ufid_next(struct m0_uint128 *ufid) {
    return m0_ufid_next(&s3_ufid_generator, 1, ufid);
  }
};
#endif

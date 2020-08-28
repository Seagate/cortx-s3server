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

#ifndef __S3_SERVER_S3_MOTR_CONTEXT_H__
#define __S3_SERVER_S3_MOTR_CONTEXT_H__

#include "s3_common.h"
#include "s3_log.h"
#include <set>

EXTERN_C_BLOCK_BEGIN

#include "motr/init.h"
#include "module/instance.h"

#include "motr/client.h"

EXTERN_C_BLOCK_END

#include "s3_memory_pool.h"

struct s3_motr_obj_context {
  struct m0_obj *objs;
  size_t n_initialized_contexts;
  size_t obj_count;
};

// To create a basic motr operation
struct s3_motr_op_context {
  struct m0_op **ops;
  struct m0_op_ops *cbs;
  size_t op_count;
};

struct s3_motr_rw_op_context {
  struct m0_indexvec *ext;
  struct m0_bufvec *data;
  struct m0_bufvec *attr;
  size_t unit_size;
  bool allocated_bufs;  // Do we own data bufs and we should free?
};

struct s3_motr_idx_context {
  struct m0_idx *idx;
  size_t n_initialized_contexts;
  size_t idx_count;
};

struct s3_motr_idx_op_context {
  struct m0_op **ops;
  struct m0_op *sync_op;
  struct m0_op_ops *cbs;
  size_t op_count;
};

struct s3_motr_kvs_op_context {
  struct m0_bufvec *keys;
  struct m0_bufvec *values;
  int *rcs;  // per key return status array
};

struct s3_motr_obj_context *create_obj_context(size_t count);
int free_obj_context(struct s3_motr_obj_context *ctx);

struct s3_motr_op_context *create_basic_op_ctx(size_t op_count);
int free_basic_op_ctx(struct s3_motr_op_context *ctx);

struct s3_motr_rw_op_context *create_basic_rw_op_ctx(size_t motr_buf_count,
                                                     size_t unit_size,
                                                     bool allocate_bufs = true);
int free_basic_rw_op_ctx(struct s3_motr_rw_op_context *ctx);

struct s3_motr_idx_context *create_idx_context(size_t idx_count);
int free_idx_context(struct s3_motr_idx_context *ctx);

struct s3_motr_idx_op_context *create_basic_idx_op_ctx(int op_count);
int free_basic_idx_op_ctx(struct s3_motr_idx_op_context *ctx);

struct s3_motr_kvs_op_context *create_basic_kvs_op_ctx(int no_of_keys);
int free_basic_kvs_op_ctx(struct s3_motr_kvs_op_context *ctx);

struct m0_bufvec *index_bufvec_alloc(int nr);
void index_bufvec_free(struct m0_bufvec *bv);

#endif

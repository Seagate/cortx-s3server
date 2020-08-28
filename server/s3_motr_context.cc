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

#include <stdio.h>
#include <stdlib.h>

#include "s3_motr_context.h"
#include "s3_mem_pool_manager.h"
#include "motr_helpers.h"

extern std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
extern std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
extern std::set<struct s3_motr_idx_context *> global_motr_idx;
extern std::set<struct s3_motr_obj_context *> global_motr_obj;
extern int shutdown_motr_teardown_called;

// Helper methods to free m0_bufvec array which holds
// Memory buffers from custom memory pool
static void s3_bufvec_free_aligned(struct m0_bufvec *bufvec, size_t unit_size,
                                   bool free_bufs = true) {
  s3_log(S3_LOG_DEBUG, "",
         "s3_bufvec_free_aligned unit_size = %zu, free_bufs = %s\n", unit_size,
         (free_bufs ? "true" : "false"));
  if (free_bufs) {
    M0_PRE(unit_size > 0);
  }
  if (bufvec != NULL) {
    if (bufvec->ov_buf != NULL) {
      if (free_bufs) {
        for (uint32_t i = 0; i < bufvec->ov_vec.v_nr; ++i) {
          S3MempoolManager::get_instance()->release_buffer_for_unit_size(
              bufvec->ov_buf[i], unit_size);
          bufvec->ov_buf[i] = NULL;
        }
      }
      free(bufvec->ov_buf);
      bufvec->ov_buf = NULL;
    }
    free(bufvec->ov_vec.v_count);
    bufvec->ov_vec.v_count = NULL;
    bufvec->ov_vec.v_nr = 0;
  }
}

// Helper methods to create m0_bufvec array which holds
// Memory buffers from custom memory pool
// Each buffer size is pre-determined == size set in mempool
static int s3_bufvec_alloc_aligned(struct m0_bufvec *bufvec, uint32_t num_segs,
                                   size_t unit_size,
                                   bool allocate_bufs = true) {
  s3_log(S3_LOG_DEBUG, "",
         "s3_bufvec_alloc_aligned unit_size = %zu, allocate_bufs = %s\n",
         unit_size, (allocate_bufs ? "true" : "false"));
  M0_PRE(num_segs > 0);
  M0_PRE(bufvec != NULL);
  if (allocate_bufs) {
    M0_PRE(unit_size > 0);
  }

  bufvec->ov_buf = NULL;
  bufvec->ov_vec.v_nr = num_segs;
  bufvec->ov_vec.v_count = (m0_bcount_t *)calloc(num_segs, sizeof(m0_bcount_t));
  if (bufvec->ov_vec.v_count == NULL) {
    s3_bufvec_free_aligned(bufvec, unit_size, allocate_bufs);
    return -ENOMEM;
  }

  bufvec->ov_buf = (void **)calloc(num_segs, sizeof(void *));
  if (bufvec->ov_buf == NULL) {
    s3_bufvec_free_aligned(bufvec, unit_size, allocate_bufs);
    return -ENOMEM;
  }

  if (allocate_bufs) {
    for (uint32_t i = 0; i < num_segs; ++i) {
      bufvec->ov_buf[i] =
          (void *)S3MempoolManager::get_instance()->get_buffer_for_unit_size(
              unit_size);

      if (bufvec->ov_buf[i] == NULL) {
        s3_bufvec_free_aligned(bufvec, unit_size, allocate_bufs);
        return -ENOMEM;
      }
      bufvec->ov_vec.v_count[i] = unit_size;
    }
  }
  return 0;
}

struct s3_motr_obj_context *create_obj_context(size_t count) {
  s3_log(S3_LOG_DEBUG, "", "Entering with object count = %zu\n", count);

  struct s3_motr_obj_context *ctx = (struct s3_motr_obj_context *)calloc(
      1, sizeof(struct s3_motr_obj_context));

  ctx->objs = (struct m0_obj *)calloc(count, sizeof(struct m0_obj));
  ctx->obj_count = count;
  global_motr_obj.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;
}

int free_obj_context(struct s3_motr_obj_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");

  free(ctx->objs);
  free(ctx);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

// To create a basic motr operation
struct s3_motr_op_context *create_basic_op_ctx(size_t op_count) {
  s3_log(S3_LOG_DEBUG, "", "Entering with op_count = %zu\n", op_count);

  struct s3_motr_op_context *ctx =
      (struct s3_motr_op_context *)calloc(1, sizeof(struct s3_motr_op_context));

  ctx->ops = (struct m0_op **)calloc(op_count, sizeof(struct m0_op *));
  ctx->cbs = (struct m0_op_ops *)calloc(op_count, sizeof(struct m0_op_ops));
  ctx->op_count = op_count;

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;
}

int free_basic_op_ctx(struct s3_motr_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  if (!shutdown_motr_teardown_called) {
    global_motr_object_ops_list.erase(ctx);
    for (size_t i = 0; i < ctx->op_count; i++) {
      if (ctx->ops[i] != NULL) {
        teardown_motr_op(ctx->ops[i]);
      }
    }
    free(ctx->ops);
    free(ctx->cbs);
    free(ctx);
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

// To create a motr RW operation
// default allocate_bufs = true -> allocate memory for each buffer
struct s3_motr_rw_op_context *create_basic_rw_op_ctx(size_t motr_buf_count,
                                                     size_t unit_size,
                                                     bool allocate_bufs) {
  s3_log(S3_LOG_DEBUG, "", "Entering motr_buf_count = %zu, unit_size = %zu\n",
         motr_buf_count, unit_size);

  struct s3_motr_rw_op_context *ctx = (struct s3_motr_rw_op_context *)calloc(
      1, sizeof(struct s3_motr_rw_op_context));

  ctx->unit_size = unit_size;
  ctx->ext = (struct m0_indexvec *)calloc(1, sizeof(struct m0_indexvec));
  ctx->data = (struct m0_bufvec *)calloc(1, sizeof(struct m0_bufvec));
  ctx->attr = (struct m0_bufvec *)calloc(1, sizeof(struct m0_bufvec));

  ctx->allocated_bufs = allocate_bufs;
  int rc = s3_bufvec_alloc_aligned(ctx->data, motr_buf_count, unit_size,
                                   allocate_bufs);
  if (rc != 0) {
    free(ctx->ext);
    free(ctx->data);
    free(ctx->attr);
    free(ctx);
    s3_log(S3_LOG_DEBUG, "", "Exiting with NULL - possible out-of-memory\n");
    return NULL;
  }

  rc = m0_bufvec_alloc(ctx->attr, motr_buf_count, 1);
  if (rc != 0) {
    s3_bufvec_free_aligned(ctx->data, unit_size, allocate_bufs);
    free(ctx->data);
    free(ctx->attr);
    free(ctx->ext);
    free(ctx);
    s3_log(S3_LOG_DEBUG, "", "Exiting with NULL - possible out-of-memory\n");
    return NULL;
  }

  rc = m0_indexvec_alloc(ctx->ext, motr_buf_count);
  if (rc != 0) {
    s3_bufvec_free_aligned(ctx->data, unit_size, allocate_bufs);
    m0_bufvec_free(ctx->attr);
    free(ctx->data);
    free(ctx->attr);
    free(ctx->ext);
    free(ctx);
    s3_log(S3_LOG_DEBUG, "", "Exiting with NULL - possible out-of-memory\n");
    return NULL;
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;
}

int free_basic_rw_op_ctx(struct s3_motr_rw_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");

  s3_bufvec_free_aligned(ctx->data, ctx->unit_size, ctx->allocated_bufs);
  m0_bufvec_free(ctx->attr);
  m0_indexvec_free(ctx->ext);
  free(ctx->ext);
  free(ctx->data);
  free(ctx->attr);
  free(ctx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

/* Motr index API */
struct s3_motr_idx_context *create_idx_context(size_t idx_count) {
  s3_log(S3_LOG_DEBUG, "", "Entering with idx_count = %zu\n", idx_count);

  struct s3_motr_idx_context *ctx = (struct s3_motr_idx_context *)calloc(
      1, sizeof(struct s3_motr_idx_context));
  ctx->idx = (struct m0_idx *)calloc(idx_count, sizeof(struct m0_idx));
  ctx->idx_count = idx_count;
  global_motr_idx.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;
}

int free_idx_context(struct s3_motr_idx_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");

  free(ctx->idx);
  free(ctx);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

struct s3_motr_idx_op_context *create_basic_idx_op_ctx(int op_count) {
  s3_log(S3_LOG_DEBUG, "", "Entering with op_count = %d\n", op_count);

  struct s3_motr_idx_op_context *ctx = (struct s3_motr_idx_op_context *)calloc(
      1, sizeof(struct s3_motr_idx_op_context));
  ctx->ops = (struct m0_op **)calloc(op_count, sizeof(struct m0_op *));
  ctx->cbs = (struct m0_op_ops *)calloc(op_count, sizeof(struct m0_op_ops));
  ctx->op_count = op_count;

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;
}

int free_basic_idx_op_ctx(struct s3_motr_idx_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  if (!shutdown_motr_teardown_called) {
    global_motr_idx_ops_list.erase(ctx);

    for (size_t i = 0; i < ctx->op_count; i++) {
      if (ctx->ops[i] == NULL) {
        continue;
      }
      teardown_motr_op(ctx->ops[i]);
    }

    if (ctx->sync_op != NULL) {
      teardown_motr_op(ctx->sync_op);
    }

    free(ctx->ops);
    free(ctx->cbs);
    free(ctx);
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

struct m0_bufvec *index_bufvec_alloc(int nr) {
  struct m0_bufvec *bv;
  bv = (m0_bufvec *)calloc(1, sizeof(*bv));
  if (bv == NULL) return NULL;
  bv->ov_vec.v_nr = nr;
  bv->ov_vec.v_count = (m0_bcount_t *)calloc(nr, sizeof(m0_bcount_t));
  if (bv->ov_vec.v_count == NULL) {
    goto FAIL;
  }
  bv->ov_buf = (void **)calloc(nr, sizeof(char *));
  if (bv->ov_buf == NULL) {
    goto FAIL;
  }

  return bv;

FAIL:
  if (bv != NULL) {
    if (bv->ov_buf != NULL) {
      free(bv->ov_buf);
    }
    if (bv->ov_vec.v_count != NULL) {
      free(bv->ov_vec.v_count);
    }
    free(bv);
  }
  return NULL;
}

void index_bufvec_free(struct m0_bufvec *bv) {
  uint32_t i;
  if (bv == NULL) return;

  if (bv->ov_buf != NULL) {
    for (i = 0; i < bv->ov_vec.v_nr; ++i) free(bv->ov_buf[i]);
    free(bv->ov_buf);
  }
  free(bv->ov_vec.v_count);
  free(bv);
}

struct s3_motr_kvs_op_context *create_basic_kvs_op_ctx(int no_of_keys) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  s3_log(S3_LOG_DEBUG, "", "no of keys = %d\n", no_of_keys);

  struct s3_motr_kvs_op_context *ctx = (struct s3_motr_kvs_op_context *)calloc(
      1, sizeof(struct s3_motr_kvs_op_context));

  ctx->keys = index_bufvec_alloc(no_of_keys);
  if (ctx->keys == NULL) goto FAIL;
  ctx->values = index_bufvec_alloc(no_of_keys);
  if (ctx->values == NULL) goto FAIL;
  ctx->rcs = (int *)calloc(no_of_keys, sizeof(int));
  if (ctx->rcs == NULL) goto FAIL;
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;

FAIL:
  if (ctx->keys) {
    index_bufvec_free(ctx->keys);
  }
  if (ctx->values) {
    index_bufvec_free(ctx->values);
  }
  if (ctx->rcs) {
    free(ctx->rcs);
  }
  if (ctx) {
    free(ctx);
  }
  return NULL;
}

int free_basic_kvs_op_ctx(struct s3_motr_kvs_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");

  index_bufvec_free(ctx->keys);
  index_bufvec_free(ctx->values);
  free(ctx->rcs);
  free(ctx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

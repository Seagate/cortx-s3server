/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include <stdio.h>
#include <stdlib.h>

#include "s3_clovis_context.h"

extern MemoryPoolHandle g_clovis_read_mem_pool_handle;

// Helper methods to free m0_bufvec array which holds
// Memory buffers from custom memory pool
static void s3_bufvec_free_aligned(struct m0_bufvec *bufvec) {
  if (bufvec != NULL) {
    if (bufvec->ov_buf != NULL) {
      for (uint32_t i = 0; i < bufvec->ov_vec.v_nr; ++i) {
        mempool_releasebuffer(g_clovis_read_mem_pool_handle, bufvec->ov_buf[i]);
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
// mempool = g_clovis_read_mem_pool_handle
static int s3_bufvec_alloc_aligned(struct m0_bufvec *bufvec,
                                   uint32_t num_segs) {
  size_t pool_item_size = 0;

  mempool_getbuffer_size(g_clovis_read_mem_pool_handle, &pool_item_size);

  M0_PRE(num_segs > 0);
  M0_PRE(bufvec != NULL);

  bufvec->ov_buf = NULL;
  bufvec->ov_vec.v_nr = num_segs;
  bufvec->ov_vec.v_count = (m0_bcount_t *)calloc(num_segs, sizeof(m0_bcount_t));
  if (bufvec->ov_vec.v_count == NULL) {
    s3_bufvec_free_aligned(bufvec);
    return -ENOMEM;
  }

  bufvec->ov_buf = (void **)calloc(num_segs, sizeof(void *));
  if (bufvec->ov_buf == NULL) {
    s3_bufvec_free_aligned(bufvec);
    return -ENOMEM;
  }

  for (uint32_t i = 0; i < num_segs; ++i) {
    bufvec->ov_buf[i] = (void *)mempool_getbuffer(g_clovis_read_mem_pool_handle,
                                                  ZEROED_ALLOCATION);

    if (bufvec->ov_buf[i] == NULL) {
      s3_bufvec_free_aligned(bufvec);
      return -ENOMEM;
    }
    bufvec->ov_vec.v_count[i] = pool_item_size;
  }
  return 0;
}

// To create a basic clovis operation
struct s3_clovis_op_context *create_basic_op_ctx(size_t op_count) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "op_count = %zu\n", op_count);
  struct s3_clovis_op_context *ctx = (struct s3_clovis_op_context *)calloc(
      1, sizeof(struct s3_clovis_op_context));

  ctx->ops =
      (struct m0_clovis_op **)calloc(op_count, sizeof(struct m0_clovis_op *));

  ctx->cbs = (struct m0_clovis_op_ops *)calloc(op_count,
                                               sizeof(struct m0_clovis_op_ops));

  ctx->obj =
      (struct m0_clovis_obj *)calloc(op_count, sizeof(struct m0_clovis_obj));
  ctx->op_count = op_count;
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return ctx;
}

int free_basic_op_ctx(struct s3_clovis_op_context *ctx) {
  size_t i;
  s3_log(S3_LOG_DEBUG, "Entering\n");
  for (i = 0; i < ctx->op_count; i++) {
    if (ctx->ops[i] != NULL) {
      m0_clovis_op_fini(ctx->ops[i]);
      m0_clovis_op_free(ctx->ops[i]);
      m0_clovis_obj_fini(&ctx->obj[i]);
    }
  }

  free(ctx->ops);
  free(ctx->cbs);
  free(ctx->obj);
  free(ctx);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return 0;
}

// To create a clovis RW operation
struct s3_clovis_rw_op_context *create_basic_rw_op_ctx(
    size_t clovis_block_count) {
  int rc = 0;
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "clovis_block_count = %zu\n", clovis_block_count);

  struct s3_clovis_rw_op_context *ctx =
      (struct s3_clovis_rw_op_context *)calloc(
          1, sizeof(struct s3_clovis_rw_op_context));

  ctx->ext = (struct m0_indexvec *)calloc(1, sizeof(struct m0_indexvec));
  ctx->data = (struct m0_bufvec *)calloc(1, sizeof(struct m0_bufvec));
  ctx->attr = (struct m0_bufvec *)calloc(1, sizeof(struct m0_bufvec));

  rc = s3_bufvec_alloc_aligned(ctx->data, clovis_block_count);
  if (rc != 0) {
    free(ctx->ext);
    free(ctx->data);
    free(ctx->attr);
    free(ctx);
    return NULL;
  }

  rc = m0_bufvec_alloc(ctx->attr, clovis_block_count, 1);
  if (rc != 0) {
    s3_bufvec_free_aligned(ctx->data);
    free(ctx->data);
    free(ctx->attr);
    free(ctx->ext);
    free(ctx);
    return NULL;
  }
  rc = m0_indexvec_alloc(ctx->ext, clovis_block_count);
  if (rc != 0) {
    s3_bufvec_free_aligned(ctx->data);
    m0_bufvec_free(ctx->attr);
    free(ctx->data);
    free(ctx->attr);
    free(ctx->ext);
    free(ctx);
    return NULL;
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return ctx;
}

int free_basic_rw_op_ctx(struct s3_clovis_rw_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  s3_bufvec_free_aligned(ctx->data);
  m0_bufvec_free(ctx->attr);
  m0_indexvec_free(ctx->ext);
  free(ctx->ext);
  free(ctx->data);
  free(ctx->attr);
  free(ctx);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return 0;
}

/* Clovis index API */
struct s3_clovis_idx_op_context *create_basic_idx_op_ctx(int idx_count) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "idx_count = %d\n", idx_count);

  struct s3_clovis_idx_op_context *ctx =
      (struct s3_clovis_idx_op_context *)calloc(
          1, sizeof(struct s3_clovis_idx_op_context));

  ctx->idx =
      (struct m0_clovis_idx *)calloc(idx_count, sizeof(struct m0_clovis_idx));
  ctx->ops =
      (struct m0_clovis_op **)calloc(idx_count, sizeof(struct m0_clovis_op *));
  ctx->cbs = (struct m0_clovis_op_ops *)calloc(idx_count,
                                               sizeof(struct m0_clovis_op_ops));

  ctx->idx_count = idx_count;
  s3_log(S3_LOG_DEBUG, "Exiting\n");

  return ctx;
}

int free_basic_idx_op_ctx(struct s3_clovis_idx_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  size_t i = 0;
  for (i = 0; i < ctx->idx_count; i++) {
    if (ctx->ops[i] == NULL) {
      continue;
    }
    m0_clovis_op_fini(ctx->ops[i]);
    m0_clovis_op_free(ctx->ops[i]);
  }
  if (ctx->idx != NULL && ctx->idx->in_entity.en_sm.sm_state != 0) {
    m0_clovis_idx_fini(ctx->idx);
  }
  free(ctx->ops);
  free(ctx->cbs);
  free(ctx->idx);
  free(ctx);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
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

struct s3_clovis_kvs_op_context *create_basic_kvs_op_ctx(int no_of_keys) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "no of keys = %d\n", no_of_keys);

  struct s3_clovis_kvs_op_context *ctx =
      (struct s3_clovis_kvs_op_context *)calloc(
          1, sizeof(struct s3_clovis_kvs_op_context));

  ctx->keys = index_bufvec_alloc(no_of_keys);
  if (ctx->keys == NULL) goto FAIL;
  ctx->values = index_bufvec_alloc(no_of_keys);
  if (ctx->values == NULL) goto FAIL;
  ctx->rcs = (int *)calloc(no_of_keys, sizeof(int));
  if (ctx->rcs == NULL) goto FAIL;
  s3_log(S3_LOG_DEBUG, "Exiting\n");
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

int free_basic_kvs_op_ctx(struct s3_clovis_kvs_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  index_bufvec_free(ctx->keys);
  index_bufvec_free(ctx->values);
  free(ctx->rcs);
  free(ctx);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return 0;
}

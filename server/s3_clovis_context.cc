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

#include <stdlib.h>
#include <stdio.h>

#include "s3_clovis_context.h"

// To create a basic clovis operation
struct s3_clovis_op_context* create_basic_op_ctx(size_t op_count) {
  printf("Called create_basic_op_ctx with op_count = %zu\n", op_count);
  struct s3_clovis_op_context* ctx = (struct s3_clovis_op_context*)calloc(op_count, sizeof(struct s3_clovis_op_context));

  ctx->ops = (struct m0_clovis_op **)calloc(op_count, sizeof(struct m0_clovis_op *));

  ctx->cbs = (struct m0_clovis_op_cbs *)calloc(op_count, sizeof(struct m0_clovis_op_cbs));

  ctx->obj = (struct m0_clovis_obj *)calloc(op_count, sizeof(struct m0_clovis_obj));
  ctx->op_count = op_count;
  return ctx;
}

int free_basic_op_ctx(struct s3_clovis_op_context *ctx) {
  size_t i;
  printf("Called free_basic_op_ctx\n");
  for (i = 0; i < ctx->op_count; i++) {
    if(ctx->ops[i] == NULL)
      continue;
    m0_clovis_op_fini(ctx->ops[i]);
    m0_clovis_op_free(ctx->ops[i]);
  }
  if(ctx->obj != NULL)
    m0_clovis_entity_fini(&ctx->obj->ob_entity);
  free(ctx->ops);
  free(ctx->cbs);
  free(ctx->obj);
  free(ctx);
  return 0;
}

// To create a clovis RW operation
struct s3_clovis_rw_op_context*
create_basic_rw_op_ctx(size_t clovis_block_count, size_t clovis_block_size) {

  int rc = 0;
  printf("Called create_basic_rw_op_ctx with clovis_block_count = %zu and clovis_block_size = %zu\n", clovis_block_count, clovis_block_size);

  struct s3_clovis_rw_op_context* ctx = (struct s3_clovis_rw_op_context*)calloc(1, sizeof(struct s3_clovis_rw_op_context));

  ctx->ext = (struct m0_indexvec *)calloc(1, sizeof(struct m0_indexvec));
  ctx->data = (struct m0_bufvec *)calloc(1, sizeof(struct m0_bufvec));
  ctx->attr = (struct m0_bufvec *)calloc(1, sizeof(struct m0_bufvec));

  rc = m0_bufvec_alloc(ctx->data, clovis_block_count, clovis_block_size);
  if (rc != 0) {
    free(ctx);
    return NULL;
  }

  rc = m0_bufvec_alloc(ctx->attr, clovis_block_count, 1);
  if (rc != 0) {
    free(ctx->data);
    free(ctx);
    return NULL;
  }
  rc = m0_indexvec_alloc(ctx->ext, clovis_block_count);
  if (rc != 0) {
    free(ctx->data);
    free(ctx->attr);
    free(ctx);
    return NULL;
  }
  return ctx;
}

int free_basic_rw_op_ctx(struct s3_clovis_rw_op_context *ctx) {
  printf("Called free_basic_rw_op_ctx\n");

  free(ctx->ext);
  free(ctx->data);
  free(ctx->attr);
  free(ctx);
  return 0;
}

/* Clovis index API */
struct s3_clovis_idx_op_context* create_basic_idx_op_ctx(int idx_count) {
  printf("Called create_basic_idx_op_ctx with idx_count = %d\n", idx_count);

  struct s3_clovis_idx_op_context* ctx = (struct s3_clovis_idx_op_context*)calloc(1, sizeof(struct s3_clovis_idx_op_context));

  ctx->idx = (struct m0_clovis_idx *)calloc(idx_count, sizeof(struct m0_clovis_idx));
  ctx->ops = (struct m0_clovis_op **)calloc(idx_count, sizeof(struct m0_clovis_op *));
  ctx->cbs = (struct m0_clovis_op_cbs *)calloc(idx_count, sizeof(struct m0_clovis_op_cbs));

  ctx->idx_count = idx_count;

  return ctx;
}

int free_basic_idx_op_ctx(struct s3_clovis_idx_op_context *ctx) {
  printf("Called free_basic_idx_op_ctx\n");
  size_t i = 0;
  for (i = 0; i < ctx->idx_count; i++) {
    if(ctx->ops[i] == NULL) {
      continue;
    }
    m0_clovis_op_fini(ctx->ops[i]);
    m0_clovis_op_free(ctx->ops[i]);
  }
  if(ctx->idx != NULL && ctx->idx->in_entity.en_sm.sm_state != 0) {
    m0_clovis_entity_fini(&ctx->idx->in_entity);
  }
  free(ctx->ops);
  free(ctx->cbs);
  free(ctx->idx);
  free(ctx);
  return 0;
}

static struct m0_bufvec* idx_bufvec_alloc(int nr)
{
	struct m0_bufvec *bv;

  bv = (m0_bufvec*)calloc(1, sizeof(*bv));
	if (bv == NULL)
		return NULL;

	bv->ov_vec.v_nr = nr;
	bv->ov_vec.v_count = (m0_bcount_t *)calloc(nr, sizeof(m0_bcount_t));
	if (bv->ov_vec.v_count == NULL)
		goto FAIL;

	bv->ov_buf = (void **)calloc(nr, sizeof(char *));
	if (bv->ov_buf == NULL)
		goto FAIL;

	return bv;

FAIL:
	m0_bufvec_free(bv);
	return NULL;
}

static void idx_bufvec_free(struct m0_bufvec *bv)
{
	uint32_t i;

	if (bv != NULL)
		return;

	if (bv->ov_buf != NULL) {
		for (i = 0; i < bv->ov_vec.v_nr; ++i)
			free(bv->ov_buf[i]);
		free(bv->ov_buf);
	}
	free(bv->ov_vec.v_count);
	free(bv);
}

struct s3_clovis_kvs_op_context*
create_basic_kvs_op_ctx(int no_of_keys) {
  printf("Called create_basic_kvs_op_ctx with no of keys = %d\n", no_of_keys);

  struct s3_clovis_kvs_op_context* ctx = (struct s3_clovis_kvs_op_context*)calloc(1, sizeof(struct s3_clovis_kvs_op_context));

  ctx->keys = idx_bufvec_alloc(no_of_keys);
  if (ctx->keys == NULL)
    goto FAIL;
  ctx->values = idx_bufvec_alloc(no_of_keys);
  if (ctx->values == NULL)
    goto FAIL;

  return ctx;

FAIL:
  if(ctx)
    free(ctx);
  return NULL;
}

int free_basic_kvs_op_ctx(struct s3_clovis_kvs_op_context *ctx) {
  printf("Called free_basic_kvs_op_ctx\n");

  idx_bufvec_free(ctx->keys);
  idx_bufvec_free(ctx->values);
  free(ctx);
  return 0;
}

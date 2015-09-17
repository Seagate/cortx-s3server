
#include <stdlib.h>
#include <stdio.h>

#include "s3_clovis_context.h"

// To create a basic clovis operation
struct s3_clovis_op_context* create_basic_op_ctx(size_t op_count) {
  printf("Calling create_basic_op_ctx with op_count = %zu\n", op_count);
  struct s3_clovis_op_context* ctx = (struct s3_clovis_op_context*)calloc(op_count, sizeof(struct s3_clovis_op_context));

  ctx->ops = (struct m0_clovis_op **)calloc(op_count, sizeof(struct m0_clovis_op *));

  ctx->cbs = (struct m0_clovis_op_cbs *)calloc(op_count, sizeof(struct m0_clovis_op_cbs));

  ctx->obj = (struct m0_clovis_obj *)calloc(op_count, sizeof(struct m0_clovis_obj));
  ctx->op_count = op_count;
  return ctx;
}

int free_basic_op_ctx(struct s3_clovis_op_context *ctx) {
  size_t i;
  printf("Calling free_basic_op_ctx\n");
  for (i = 0; i < ctx->op_count; i++) {
    m0_clovis_op_fini(ctx->ops[i]);
    m0_clovis_op_free(ctx->ops[i]);
  }
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
  printf("Calling create_basic_rw_op_ctx with clovis_block_count = %zu and clovis_block_size = %zu\n", clovis_block_count, clovis_block_size);

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
  printf("Calling free_basic_rw_op_ctx\n");

  free(ctx->ext);
  free(ctx->data);
  free(ctx->attr);
  free(ctx);
  return 0;
}

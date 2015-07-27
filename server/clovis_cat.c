#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#include "clovis_helpers.h"

extern struct m0_clovis_scope     clovis_uber_scope;

int clovis_read(struct m0_uint128 object_id, int clovis_block_size,
      int start_block_idx, int clovis_block_count, struct m0_bufvec *data)
{
  int                     i;
  int                     rc;
  struct m0_clovis_op    *ops[1] = {NULL};
  struct m0_clovis_obj    obj;
  uint64_t                last_index;
  struct m0_indexvec      ext;
  // struct m0_bufvec        data;
  struct m0_bufvec        attr;

  /* we want to read <clovis_block_count> from the beginning of the object */
  rc = m0_indexvec_alloc(&ext, clovis_block_count);
  if (rc != 0)
    return -1;

  /*
   * this allocates <clovis_block_count> * 4K buffers for data, and initialises
   * the bufvec for us.
   */
  rc = m0_bufvec_alloc(data, clovis_block_count, clovis_block_size);
  if (rc != 0)
    return -1;
  rc = m0_bufvec_alloc(&attr, clovis_block_count, 1);
  if(rc != 0)
    return -1;

  last_index = clovis_block_size * start_block_idx;
  for (i = 0; i < clovis_block_count; i++) {
    ext.iv_index[i] = last_index ;
    ext.iv_vec.v_count[i] = clovis_block_size;
    last_index += clovis_block_size;
    
    /* we don't want any attributes */
    attr.ov_vec.v_count[i] = 0;

  }

  /* Read the requisite number of blocks from the entity */
  m0_clovis_obj_init(&obj, &clovis_uber_scope, &object_id);
  /* Create the read request */
  m0_clovis_obj_op(&obj, M0_CLOVIS_OC_READ, &ext, data, &attr, 0, &ops[0]);
  assert(rc == 0);
  assert(ops[0] != NULL);
  assert(ops[0]->op_sm.sm_rc == 0);

  m0_clovis_op_launch(ops, 1);

  /* wait */
  rc = m0_clovis_op_wait(ops[0],
          M0_BITS(M0_CLOVIS_OS_FAILED,
            M0_CLOVIS_OS_STABLE),
         M0_TIME_NEVER);
  if(rc != 0)
    return rc;
  assert(rc == 0);
  assert(ops[0]->op_sm.sm_state == M0_CLOVIS_OS_STABLE);
  assert(ops[0]->op_sm.sm_rc == 0);

  /* fini and release */
  m0_clovis_op_fini(ops[0]);
  m0_clovis_op_free(ops[0]);
  m0_clovis_entity_fini(&obj.ob_entity);

  m0_indexvec_free(&ext);
  // m0_bufvec_free(&data);
  m0_bufvec_free(&attr);
  
  /* for now should be freed by caller */
  // m0_bufvec_free(&data);
  return 0;
}



/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */

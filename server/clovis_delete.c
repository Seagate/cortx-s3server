#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#include "clovis_helpers.h"

extern struct m0_clovis_scope     clovis_uber_scope;

int clovis_delete(struct m0_uint128 object_id)
{
  int                     rc;
  struct m0_clovis_op    *ops[1] = {NULL};
  struct m0_clovis_obj    obj;

  /* Create an entity */
  m0_clovis_obj_init(&obj, &clovis_uber_scope, &object_id);

  // Delete op
  rc = m0_clovis_entity_delete(&obj.ob_entity, &ops[0]);

  m0_clovis_op_launch(ops, 1);

  rc = m0_clovis_op_wait(ops[0], 
    1ULL << M0_CLOVIS_OS_FAILED | 1ULL << M0_CLOVIS_OS_STABLE,
    M0_TIME_NEVER);

  /* fini and release */
  m0_clovis_op_fini(ops[0]);
  m0_clovis_op_free(ops[0]);
  m0_clovis_entity_fini(&obj.ob_entity);

  return rc;
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

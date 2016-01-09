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
 * Original author:  Rajesh Nambiar <rajesh.nambiar@seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 27-Nov-2015
 */

#include "s3_callback_test_helpers.h"
#include "s3_clovis_context.h"

void * async_success_call(void * arg) {
  struct s3_clovis_idx_op_context *idx_ctx;
  idx_ctx = (struct s3_clovis_idx_op_context *)arg;
  s3_clovis_op_stable(idx_ctx->ops[0]);
  return NULL;
}

void * async_fail_call(void * arg) {
  struct s3_clovis_idx_op_context *idx_ctx;
  idx_ctx = (struct s3_clovis_idx_op_context *)arg;
  s3_clovis_op_failed(idx_ctx->ops[0]);
  return NULL;
}


#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_RW_COMMON_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_RW_COMMON_H__

#include "s3_common.h"

EXTERN_C_BLOCK_BEGIN
#include "module/instance.h"
#include "mero/init.h"

#include "clovis/clovis.h"

/* libevhtp */
#include <evhtp.h>

void clovis_op_done_on_main_thread(evutil_socket_t, short events, void *user_data);

void s3_clovis_op_stable(struct m0_clovis_op *op);

void s3_clovis_op_failed(struct m0_clovis_op *op);
EXTERN_C_BLOCK_END

#endif

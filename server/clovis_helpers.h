 */

#pragma once

#ifndef __S3_SERVER_CLOVIS_HELPERS_H__
#define __S3_SERVER_CLOVIS_HELPERS_H__

#ifdef __cplusplus
#define EXTERN_C_BLOCK_BEGIN extern "C" {
#define EXTERN_C_BLOCK_END }
#define EXTERN_C_FUNC extern "C"
#else
#define EXTERN_C_BLOCK_BEGIN
#define EXTERN_C_BLOCK_END
#define EXTERN_C_FUNC
#endif

EXTERN_C_BLOCK_BEGIN

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <sys/stat.h>

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"
#include "helpers/helpers.h"

/* For htonl */
#include <arpa/inet.h>

EXTERN_C_BLOCK_END

int init_clovis(void);
void fini_clovis(void);
int create_new_instance_id(struct m0_uint128 *ufid);
void teardown_clovis_op(struct m0_clovis_op *op);
void teardown_clovis_wait_op(struct m0_clovis_op *op);
void global_clovis_teardown();
#endif

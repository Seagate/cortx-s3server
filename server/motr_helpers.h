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

#ifndef __S3_SERVER_MOTR_HELPERS_H__
#define __S3_SERVER_MOTR_HELPERS_H__

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

#include "motr/client.h"
#include "motr/idx.h"
#include "helpers/helpers.h"

/* For htonl */
#include <arpa/inet.h>

EXTERN_C_BLOCK_END

int init_motr(void);
void fini_motr(void);
int create_new_instance_id(struct m0_uint128 *ufid);
void teardown_motr_op(struct m0_op *op);
void teardown_motr_wait_op(struct m0_op *op);
void teardown_motr_cancel_wait_op(struct m0_op *op, int sec = -1);
void global_motr_teardown();
#endif

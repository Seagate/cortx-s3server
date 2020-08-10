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

#ifndef __S3_SERVER_S3_FI_COMMON_H__
#define __S3_SERVER_S3_FI_COMMON_H__

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
#include "lib/finject.h"

#ifdef HAVE_CONFIG_H
#include "config.h" /* ENABLE_FAULT_INJECTION */
#endif

typedef struct m0_fi_fault_point s3_fp;
#define S3_FI_FUNC_NAME "s3_fi_is_enabled"
#define S3_MODULE_NAME "UNKNOWN"

#ifdef ENABLE_FAULT_INJECTION
#include "lib/finject_internal.h"

/**
 * Creates and checks fault point in code, identified by "tag"
 *
 * @param tag  FP tag, which can be activated using s3_fi_enable* routines
 *
 * eg: s3_fi_is_enabled("write_fail");
 *
 */
int s3_fi_is_enabled(const char *tag);

/**
 * Enables fault point, which identified by "tag"
 *
 * @param tag  FP tag, which was specified as a parameter to s3_fi_is_enabled()
 *
 * eg: s3_fi_enable("write_fail");
 *
 */
void s3_fi_enable(const char *tag);

/**
 * Enables fault point, which identified by "tag".
 * Fault point is triggered only once.
 *
 * @param tag  FP tag, which was specified as a parameter to s3_fi_is_enabled()
 *
 * eg: s3_fi_enable("write_fail");
 *
 */
void s3_fi_enable_once(const char *tag);

/**
 * Enables fault point, which identified by "tag"
 *
 * @param tag  FP tag, which was specified as a parameter to s3_fi_is_enabled()
 * @param p    Integer number in range [1..100], which means a probability in
 *             percents, with which FP should be triggered on each hit
 * eg: s3_fi_enable("write_fail", 10);
 *
 */
void s3_fi_enable_random(const char *tag, uint32_t p);

/**
 * Enables fault point, which identified by "tag"
 *
 * @param tag  FP tag, which was specified as a parameter to s3_fi_is_enabled()
 * @param n    A "frequency" with which FP is triggered
 * eg: s3_fi_enable("write_fail", 5);
 * Here fault injection is triggered first 5 times.
 *
 */
void s3_fi_enable_each_nth_time(const char *tag, uint32_t n);

/**
 * Enables fault point, which identified by "tag"
 *
 * @param tag  FP tag, which was specified as a parameter to s3_fi_is_enabled()
 * @param n    Integer values, means 'skip triggering N times in a row and then
 *trigger
 *             M times in a row
 * @param m    Integer values, means M times in a row to trigger FP, after
 *skipping it
 *             N times before
 * eg: s3_fi_enable("write_fail", 5, 20);
 * Here FI is not trigerred first 5 times then FI is triggered next 20 times,
 * this cycle is repeated.
 */
void s3_fi_enable_off_n_on_m(const char *tag, uint32_t n, uint32_t m);

/**
 * Enables fault point, which identified by "tag"
 *
 * @param tag  FP tag, which was specified as a parameter to s3_fi_is_enabled()

 * eg: s3_fi_enable("write_fail", 10, 200);
 *
 */
void s3_fi_disable(const char *fp_tag);

#else /* ENABLE_FAULT_INJECTION */

inline int s3_fi_is_enabled(const char *tag) { return false; }

inline void s3_fi_enable(const char *tag) {}

inline void s3_fi_enable_once(const char *tag) {}

inline void s3_fi_enable_random(const char *tag, uint32_t p) {}

inline void s3_fi_enable_each_nth_time(const char *tag, uint32_t n) {}

inline void s3_fi_enable_off_n_on_m(const char *tag, uint32_t n, uint32_t m) {}

inline void s3_fi_disable(const char *fp_tag) {}

#endif /* ENABLE_FAULT_INJECTION */

EXTERN_C_BLOCK_END

#endif /*  __S3_SERVER_S3_FI_COMMON_H__ */

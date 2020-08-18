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

#include <stdio.h>
#include <addb2/plugin_api.h>

#include "s3_addb_plugin_auto.h"
#include "s3_addb_map.h"

/* Borrowed from addb2/dump.c, hope Motr will publish it as API in future */

static void dec(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%" PRId64, v[0]);
}

static void hex(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%" PRIx64, v[0]);
}

static void hex0x(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "0x%" PRIx64, v[0]);
}

static void oct(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%" PRIo64, v[0]);
}

static void ptr(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "@%p", *(void **)v);
}

static void bol(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%s", v[0] ? "true" : "false");
}

/* end of clip from dump.c */

static void idx_to_state(struct m0_addb2__context *ctx, const uint64_t *v,
                         char *buf) {
  const char *state_name = addb_idx_to_s3_state(*v);
  sprintf(buf, "%s", state_name);
}

static void msrm(struct m0_addb2__context *ctx, const uint64_t *v, char *buf) {
  const char *msrm_name = addb_measurement_to_name(*v);
  sprintf(buf, "%s", msrm_name);
}

static struct m0_addb2__id_intrp gs_curr_ids[] = {
    {S3_ADDB_REQUEST_ID,
     "s3-request-uid",
     {&dec, &hex0x, &hex0x},
     {"s3_request_id", "uid_first_64_bits", "uid_last_64_bits"}, },
    {S3_ADDB_REQUEST_TO_MOTR_ID,
     "s3-request-to-motr",
     {&dec, &dec},
     {"s3_request_id", "motr_id"}},
    {S3_ADDB_FIRST_REQUEST_ID,
     "s3-request-state",
     {&dec, &idx_to_state},
     {"s3_request_id", "state"},
     .ii_repeat = (S3_ADDB_LAST_REQUEST_ID - S3_ADDB_FIRST_REQUEST_ID)},
    {S3_ADDB_MEASUREMENT_ID, "s3-measurement",
     {&msrm, &dec, &dec, &dec, &dec, &dec, &dec, &dec,
      &dec,  &dec, &dec, &dec, &dec, &dec, &dec}},
    {0}};

int m0_addb2_load_interps(uint64_t flags,
                          struct m0_addb2__id_intrp **intrps_array) {
  /* suppres "unused" warnings */
  (void)dec;
  (void)hex0x;
  (void)oct;
  (void)hex;
  (void)bol;
  (void)ptr;

  *intrps_array = gs_curr_ids;
  return 0;
}

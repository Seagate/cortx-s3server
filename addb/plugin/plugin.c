/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Ivan Tishchenko   <ivan.tishchenko@seagate.com>
 * Original creation date: 23-Oct-2019
 */

#include <stdio.h>
#include <inttypes.h>

/* #include <addb2/plugin_api.h> */
/* Waiting till Mero team lands their change, see
 * http://gerrit.mero.colo.seagate.com:8080/#/c/18477/ */

/* definitions from not-yet-merged mero change, to be able to build this
 * plugin (copied from <addb2/pluginapi.h>: */

enum {
  FIELD_MAX = 15
};

struct context;

struct m0_id_intrp {
  uint64_t ii_id;
  const char *ii_name;
  void (*ii_print[FIELD_MAX])(struct context *ctx, const uint64_t *v,
                              char *buf);
  const char *ii_field[FIELD_MAX];
  void (*ii_spec)(struct context *ctx, char *buf);
  int ii_repeat;
};

int m0_addb2_load_interps(uint64_t flags, struct m0_id_intrp **intrps_array);

/* end of definitions from not-yet-merged mero change <addb2/plugin_api.h> */

#include "s3_addb_plugin_auto.h"

/* Borrowed from addb2/dump.c, hope Mero will publish it as API in future */

static void dec(struct context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%" PRId64, v[0]);
}

static void hex(struct context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%" PRIx64, v[0]);
}

static void hex0x(struct context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "0x%" PRIx64, v[0]);
}

static void oct(struct context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%" PRIo64, v[0]);
}

static void ptr(struct context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "@%p", *(void **)v);
}

static void bol(struct context *ctx, const uint64_t *v, char *buf) {
  sprintf(buf, "%s", v[0] ? "true" : "false");
}

/* end of clip from dump.c */

static struct m0_id_intrp gs_curr_ids[] = {
    {S3_ADDB_REQUEST_ID,
     "s3-request-uid",
     {&dec, &hex0x, &hex0x},
     {"s3_request_id", "uid_first_64_bits", "uid_last_64_bits"}, },
    {S3_ADDB_REQUEST_TO_CLOVIS_ID,
     "s3-request-to-clovis",
     {&dec, &dec},
     {"s3_request_id", "clovis_id"}},
    {S3_ADDB_FIRST_REQUEST_ID,
     "s3-request-state",
     {&dec, &dec},
     {"s3_request_id", "state"},
     .ii_repeat = (S3_ADDB_LAST_REQUEST_ID - S3_ADDB_FIRST_REQUEST_ID)},
    {0}};

int m0_addb2_load_interps(uint64_t flags, struct m0_id_intrp **intrps_array) {
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

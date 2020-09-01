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

#ifdef __cplusplus
#define EXTERN_C_BLOCK_BEGIN extern "C" {
#define EXTERN_C_BLOCK_END }
#define EXTERN_C_FUNC extern "C"
#else
#define EXTERN_C_BLOCK_BEGIN
#define EXTERN_C_BLOCK_END
#define EXTERN_C_FUNC
#endif

#define __STDC_FORMAT_MACROS
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
#include "lib/memory.h"
#include "lib/types.h"

#include "fid/fid.h"
/* For htonl */
#include <arpa/inet.h>

EXTERN_C_BLOCK_END

#include <gflags/gflags.h>
#include <inttypes.h>

/* Motr parameters */

DEFINE_string(action, "none",
              "KVS action:get,next,put,del,createidx,deleteidx");
DEFINE_string(op_oid, "", "KVS operation oid");
DEFINE_string(key, "", "KVS operation key");
DEFINE_string(value, "", "KVS operation value");
DEFINE_int32(op_count, 10, "number of keys");
DEFINE_string(index_hi, "", "KVS operation oid HI <0xhex64bitOidString>");
DEFINE_string(index_lo, "", "KVS operation oid LO <0xhex64bitOidString>");
DEFINE_int32(kvstore, 2, "Type of index service: 1:MOTR; 2:CASSANDRA");

DEFINE_string(motr_local_addr, "local@tcp:12345:33:100", "Motr local address");
DEFINE_string(motr_ha_addr, "local@tcp:12345:34:1", "Motr ha address");
DEFINE_string(motr_profile, "<0x7000000000000001:0>", "Motr profile");
DEFINE_string(motr_proc, "<0x7200000000000000:0>", "Motr proc");
DEFINE_string(motr_kvs_keyspace, "motr_index_keyspace", "Motr keyspace");
DEFINE_string(motr_cluster_ep, "127.0.0.1", "Cluster EP");
DEFINE_int32(recv_queue_min_len, 16,
             "Recv Queue min length: default 16");  // As Suggested by Motr team
DEFINE_int32(max_rpc_msg_size, 65536,
             "RPC msg size max: default 65536");  // As Suggested by Motr team

static struct m0_idx_dix_config dix_conf;
static struct m0_idx_cass_config cass_conf;
static struct m0_client *motr_instance = NULL;
static struct m0_container motr_container;
static struct m0_realm motr_uber_realm;
static struct m0_config motr_conf;

struct m0_uint128 root_user_index_oid;
struct m0_uint128 root_user_bucket_list_oid;

static int init_motr(void) {
  int rc;

  motr_conf.mc_is_oostore = true;
  motr_conf.mc_is_read_verify = false;
  motr_conf.mc_local_addr = FLAGS_motr_local_addr.c_str();
  motr_conf.mc_ha_addr = FLAGS_motr_ha_addr.c_str();
  motr_conf.mc_profile = FLAGS_motr_profile.c_str();
  motr_conf.mc_process_fid = FLAGS_motr_proc.c_str();
  motr_conf.mc_tm_recv_queue_min_len = FLAGS_recv_queue_min_len;
  motr_conf.mc_max_rpc_msg_size = FLAGS_max_rpc_msg_size;

  cass_conf.cc_cluster_ep = const_cast<char *>(FLAGS_motr_cluster_ep.c_str());
  cass_conf.cc_keyspace = const_cast<char *>(FLAGS_motr_kvs_keyspace.c_str());

  cass_conf.cc_max_column_family_num = 1;
  dix_conf.kc_create_meta = false;
  motr_conf.mc_idx_service_id = FLAGS_kvstore;
  if (FLAGS_kvstore == 2)
    motr_conf.mc_idx_service_conf = &cass_conf;
  else
    motr_conf.mc_idx_service_conf = &dix_conf;

  motr_conf.mc_layout_id = 0;

  /* Motr instance */
  rc = m0_client_init(&motr_instance, &motr_conf, true);
  if (rc != 0) {
    fprintf(stderr, "Failed to initilise Motr\n");
    goto err_exit;
  }

  /* And finally, motr root realm */
  m0_container_init(&motr_container, NULL, &M0_UBER_REALM, motr_instance);

  motr_uber_realm = motr_container.co_realm;
  return 0;

err_exit:
  return rc;
}

static void fini_motr(void) { m0_client_fini(motr_instance, true); }

static int create_index(struct m0_uint128 id) {
  int rc;
  struct m0_idx idx;
  // struct m0_obj obj;
  struct m0_op *ops[1] = {NULL};

  memset(&idx, 0, sizeof(struct m0_idx));

  m0_idx_init(&idx, &motr_uber_realm, &id);
  m0_entity_create(NULL, &idx.in_entity, &ops[0]);

  m0_op_launch(ops, ARRAY_SIZE(ops));

  rc = m0_op_wait(ops[0], M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
                  m0_time_from_now(3, 0));
  if (rc < 0) fprintf(stderr, "Motr Operation failed:%d\n", rc);
  if (ops[0]->op_sm.sm_state != M0_OS_STABLE)
    fprintf(stderr, "Index Operation failed:%d\n", ops[0]->op_sm.sm_state);

  m0_op_fini(ops[0]);
  m0_op_free(ops[0]);
  m0_entity_fini(&idx.in_entity);

  return rc;
}

static int delete_index(struct m0_uint128 id) {
  int rc;
  struct m0_idx idx;
  struct m0_op *ops[1] = {NULL};

  memset(&idx, 0, sizeof(struct m0_idx));

  m0_idx_init(&idx, &motr_uber_realm, &id);
  m0_entity_open(&idx.in_entity, &ops[0]);
  m0_entity_delete(&idx.in_entity, &ops[0]);

  m0_op_launch(ops, ARRAY_SIZE(ops));

  rc = m0_op_wait(ops[0], M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
                  m0_time_from_now(3, 0));

  if (rc < 0) fprintf(stderr, "Motr Operation failed:%d\n", rc);
  if (ops[0]->op_sm.sm_state != M0_OS_STABLE)
    fprintf(stderr, "Index Operation failed:%d\n", ops[0]->op_sm.sm_state);

  m0_op_fini(ops[0]);
  m0_op_free(ops[0]);
  m0_entity_fini(&idx.in_entity);

  return rc;
}

static void idx_bufvec_free(struct m0_bufvec *bv) {
  uint32_t i;
  if (bv == NULL) return;

  if (bv->ov_buf != NULL) {
    for (i = 0; i < bv->ov_vec.v_nr; ++i) free(bv->ov_buf[i]);
    free(bv->ov_buf);
  }
  free(bv->ov_vec.v_count);
  free(bv);
}

static struct m0_bufvec *idx_bufvec_alloc(int nr) {
  struct m0_bufvec *bv;

  bv = (m0_bufvec *)m0_alloc(sizeof(*bv));
  if (bv == NULL) return NULL;

  bv->ov_vec.v_nr = nr;
  bv->ov_vec.v_count = (m0_bcount_t *)calloc(nr, sizeof(bv->ov_vec.v_count[0]));
  if (bv->ov_vec.v_count == NULL) goto FAIL;

  bv->ov_buf = (void **)calloc(nr, sizeof(bv->ov_buf[0]));
  if (bv->ov_buf == NULL) goto FAIL;

  return bv;

FAIL:
  if (bv != NULL && bv->ov_vec.v_count != NULL) {
    free(bv->ov_vec.v_count);
  }
  m0_bufvec_free(bv);
  return NULL;
}

// NOTE: Common func to query opcodes supported via m0_idx_opcode
// Refer motr/client.h for opcodes
static int execute_kv_query(struct m0_uint128 id, struct m0_bufvec *keys,
                            int *rc_keys, struct m0_bufvec *vals,
                            unsigned int flags, enum m0_idx_opcode opcode) {
  int rc;
  struct m0_op *ops[1] = {NULL};
  struct m0_idx idx;

  memset(&idx, 0, sizeof(idx));
  ops[0] = NULL;

  m0_idx_init(&idx, &motr_uber_realm, &id);
  m0_idx_op(&idx, opcode, keys, vals, rc_keys, flags, &ops[0]);

  m0_op_launch(ops, 1);
  rc = m0_op_wait(ops[0], M0_BITS(M0_OS_FAILED, M0_OS_STABLE), M0_TIME_NEVER);
  if (rc < 0) {
    fprintf(stderr, "Motr op failed:%d \n", rc);
    return rc;
  }

  rc = m0_rc(ops[0]);
  /* fini and release */
  m0_op_fini(ops[0]);
  m0_op_free(ops[0]);
  m0_entity_fini(&idx.in_entity);

  return 0;
}

static int kv_next(struct m0_uint128 id, const char *keystring, int nr_kvp) {
  int rc = 0, i = 0;
  struct m0_bufvec *keys;
  struct m0_bufvec *vals;
  int *rc_keys = NULL;
  int keylen = strlen(keystring);
  void *key_ref_copy = NULL;
  int ret_key_cnt = 0;

  /* Allocate bufvec's for keys and vals. */
  keys = idx_bufvec_alloc(nr_kvp);
  vals = idx_bufvec_alloc(nr_kvp);
  rc_keys = (int *)calloc(nr_kvp, sizeof(int));

  if (keys == NULL || vals == NULL || rc_keys == NULL) {
    rc = -ENOMEM;
    fprintf(stderr, "Memory allocation failed:%d\n", rc);
    goto ERROR;
  }

  if (keylen == 0) {
    keys->ov_vec.v_count[0] = 0;
    keys->ov_buf[0] = NULL;
  } else {
    keys->ov_vec.v_count[0] = keylen;
    key_ref_copy = keys->ov_buf[0] = malloc(keylen + 1);
    memcpy(keys->ov_buf[0], keystring, keylen);
  }

  rc = execute_kv_query(id, keys, rc_keys, vals, 0, M0_IC_NEXT);
  if (rc < 0) {
    fprintf(stderr, "Index Operation failed:%d\n", rc);
    goto ERROR;
  }

  for (i = 0; i < nr_kvp && keys->ov_buf[i] != NULL; i++) {
    if (rc_keys[i] == 0) {
      ret_key_cnt++;
      fprintf(stdout, "Index: %" PRIx64 ":%" PRIx64 "\n", id.u_hi, id.u_lo);
      fprintf(stdout, "Key: %.*s\n", (int)keys->ov_vec.v_count[i],
              (char *)keys->ov_buf[i]);
      if (vals->ov_buf[i] == NULL) {
        fprintf(stdout, "Val: \n");
      } else {
        fprintf(stdout, "Val: %.*s\n", (int)vals->ov_vec.v_count[i],
                (char *)vals->ov_buf[i]);
      }
      fprintf(stdout, "----------------------------------------------\n");
    }
  }

  if (ret_key_cnt && key_ref_copy) free(key_ref_copy);

  idx_bufvec_free(keys);
  idx_bufvec_free(vals);
  if (rc_keys) free(rc_keys);

  return rc;

ERROR:
  if (keys) idx_bufvec_free(keys);
  if (vals) idx_bufvec_free(vals);
  if (rc_keys) free(rc_keys);
  return rc;
}

void fetch_root_index() {
  struct m0_uint128 temp = {0ULL, 0ULL};
  temp.u_lo = 1;
  // Fixed oid for root index -- M0_ID_APP + 1
  m0_uint128_add(&root_user_index_oid, &M0_ID_APP, &temp);
  struct m0_fid index_fid =
      M0_FID_TINIT('x', root_user_index_oid.u_hi, root_user_index_oid.u_lo);
  root_user_index_oid.u_hi = index_fid.f_container;
  root_user_index_oid.u_lo = index_fid.f_key;
}

static int kv_get(struct m0_uint128 id, const char *keystring) {
  int rc = 0;
  struct m0_bufvec *keys;
  struct m0_bufvec *vals;
  int rc_key = 0;
  int keylen = strlen(keystring);

  /* Allocate bufvec's for keys and vals. */
  keys = idx_bufvec_alloc(1);
  vals = idx_bufvec_alloc(1);
  if (keys == NULL || vals == NULL) {
    rc = -ENOMEM;
    fprintf(stderr, "Memory allocation failed:%d\n", rc);
    goto ERROR;
  }

  keys->ov_vec.v_count[0] = keylen;
  keys->ov_buf[0] = m0_alloc(keylen + 1);
  if (keys->ov_buf[0] == NULL) goto ERROR;
  memcpy(keys->ov_buf[0], keystring, keylen);

  rc = execute_kv_query(id, keys, &rc_key, vals, 0, M0_IC_GET);
  if (rc < 0 || rc_key < 0) {
    fprintf(stderr, "Index Operation failed:%d\n", rc);
    goto ERROR;
  }

  if (keys->ov_buf[0] == NULL) goto ERROR;

  fprintf(stdout, "Index:%" PRIx64 ":%" PRIx64 "\n", id.u_hi, id.u_lo);
  fprintf(stdout, "Key: %.*s\n", (int)keys->ov_vec.v_count[0],
          (char *)keys->ov_buf[0]);
  if (vals->ov_buf[0] == NULL) {
    fprintf(stdout, "Val: \n");
  } else {
    fprintf(stdout, "Val: %.*s\n", (int)vals->ov_vec.v_count[0],
            (char *)vals->ov_buf[0]);
  }
  fprintf(stdout, "----------------------------------------------\n");

  idx_bufvec_free(keys);
  idx_bufvec_free(vals);

  return rc;

ERROR:
  if (keys) idx_bufvec_free(keys);
  if (vals) idx_bufvec_free(vals);
  return rc;
}

static int kv_put(struct m0_uint128 id, const char *keystring,
                  const char *keyvalue) {
  int rc = 0;
  struct m0_bufvec *keys;
  struct m0_bufvec *vals;
  int rc_key = 0;
  int keylen = strlen(keystring);
  int valuelen = strlen(keyvalue);
  /* Allocate bufvec's for keys and vals. */
  keys = idx_bufvec_alloc(1);
  vals = idx_bufvec_alloc(1);
  if (keys == NULL || vals == NULL) {
    rc = -ENOMEM;
    fprintf(stderr, "Memory allocation failed:%d\n", rc);
    goto ERROR;
  }

  keys->ov_vec.v_count[0] = keylen;
  keys->ov_buf[0] = m0_alloc(keylen + 1);
  if (keys->ov_buf[0] == NULL) goto ERROR;
  memcpy(keys->ov_buf[0], keystring, keylen);

  vals->ov_vec.v_count[0] = valuelen;
  vals->ov_buf[0] = (char *)malloc(valuelen);
  memcpy(vals->ov_buf[0], keyvalue, valuelen);

  rc = execute_kv_query(id, keys, &rc_key, vals, 0, M0_IC_PUT);
  if (rc < 0 || rc_key < 0) {
    fprintf(stderr, "Index Operation failed:%d\n", rc);
    goto ERROR;
  }

  idx_bufvec_free(keys);
  idx_bufvec_free(vals);

  return rc;

ERROR:
  if (keys) idx_bufvec_free(keys);
  if (vals) idx_bufvec_free(vals);
  return rc;
}

static int kv_delete(struct m0_uint128 id, const char *keystring) {
  int rc = 0;
  struct m0_bufvec *keys;
  struct m0_bufvec *vals = NULL;
  int rc_key = 0;
  int keylen = strlen(keystring);

  /* Allocate bufvec's for keys and vals. */
  keys = idx_bufvec_alloc(1);
  // vals = idx_bufvec_alloc(1);
  if (keys == NULL) {
    rc = -ENOMEM;
    goto ERROR;
  }

  keys->ov_vec.v_count[0] = keylen;
  keys->ov_buf[0] = m0_alloc(keylen + 1);
  if (keys->ov_buf[0] == NULL) goto ERROR;
  memcpy(keys->ov_buf[0], keystring, keylen);

  rc = execute_kv_query(id, keys, &rc_key, vals, 0, M0_IC_DEL);
  if (rc < 0 || rc_key < 0) {
    fprintf(stderr, "Index Operation failed:%d\n", rc);
    goto ERROR;
  }

  fprintf(stdout, "Index:%" PRIx64 ":%" PRIx64 "\n", id.u_hi, id.u_lo);
  fprintf(stdout, "Key: %.*s\n", (int)keys->ov_vec.v_count[0],
          (char *)keys->ov_buf[0]);
  fprintf(stdout, "successfully deleted \n");
  fprintf(stdout, "----------------------------------------------\n");

  idx_bufvec_free(keys);
  idx_bufvec_free(vals);

  return rc;

ERROR:
  if (keys) idx_bufvec_free(keys);
  if (vals) idx_bufvec_free(vals);
  return rc;
}

static int get_kv_pair() {
  struct m0_uint128 id;

  id.u_hi = std::stoull(FLAGS_index_hi, nullptr, 0);
  id.u_lo = std::stoull(FLAGS_index_lo, nullptr, 0);

  return kv_get(id, FLAGS_key.c_str());
}

static int get_kv_next() {
  struct m0_uint128 id;

  id.u_hi = std::stoull(FLAGS_index_hi, nullptr, 0);
  id.u_lo = std::stoull(FLAGS_index_lo, nullptr, 0);

  return kv_next(id, FLAGS_key.c_str(), FLAGS_op_count);
}

static int put_kv_pair() {
  struct m0_uint128 id;

  id.u_hi = std::stoull(FLAGS_index_hi, nullptr, 0);
  id.u_lo = std::stoull(FLAGS_index_lo, nullptr, 0);

  return kv_put(id, FLAGS_key.c_str(), FLAGS_value.c_str());
}

static int create_kv_index() {
  struct m0_uint128 id;

  id.u_hi = std::stoull(FLAGS_index_hi, nullptr, 0);
  id.u_lo = std::stoull(FLAGS_index_lo, nullptr, 0);

  return create_index(id);
}

static int delete_kv_index() {
  struct m0_uint128 id;

  id.u_hi = std::stoull(FLAGS_index_hi, nullptr, 0);
  id.u_lo = std::stoull(FLAGS_index_lo, nullptr, 0);

  return delete_index(id);
}

static int delete_kv_pair() {
  struct m0_uint128 id;

  id.u_hi = std::stoull(FLAGS_index_hi, nullptr, 0);
  id.u_lo = std::stoull(FLAGS_index_lo, nullptr, 0);

  return kv_delete(id, FLAGS_key.c_str());
}

int main(int argc, char **argv) {
  int rc = 0;

  // Get input parameters

  gflags::ParseCommandLineFlags(&argc, &argv, false);

  rc = init_motr();
  if (rc < 0) {
    return rc;
  }

  if (!FLAGS_action.compare("root")) {
    fetch_root_index();
    rc = kv_next(root_user_index_oid, "", FLAGS_op_count);
  } else if (!FLAGS_action.compare("get")) {
    rc = get_kv_pair();
  } else if (!FLAGS_action.compare("next")) {
    rc = get_kv_next();
  } else if (!FLAGS_action.compare("put")) {
    rc = put_kv_pair();
  } else if (!FLAGS_action.compare("del")) {
    rc = delete_kv_pair();
  } else if (!FLAGS_action.compare("createidx")) {
    rc = create_kv_index();
  } else if (!FLAGS_action.compare("deleteidx")) {
    rc = delete_kv_index();
  } else {
    fprintf(stderr, "Unrecognised action: try --help \n");
    rc = -EINVAL;
  }

  // Clean-up
  fini_motr();
  return rc;
}

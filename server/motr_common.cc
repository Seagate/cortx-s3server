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
#include "motr_helpers.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_m0_uint128_helper.h"
#include "s3_factory.h"
#include "s3_iem.h"
#define MAX_THREAD 20

static struct m0_client *motr_instance = NULL;
struct m0_ufid_generator s3_ufid_generator;
struct m0_container motr_container;
struct m0_realm motr_uber_realm;
struct m0_config motr_conf;

static struct m0_idx_dix_config dix_conf;
static struct m0_idx_cass_config cass_conf;

const char *motr_indices = "./indices";

// extern struct m0_addb_ctx m0_addb_ctx;
extern std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
extern std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
extern std::set<struct s3_motr_idx_context *> global_motr_idx;
extern std::set<struct s3_motr_obj_context *> global_motr_obj;
extern pthread_t global_tid_objop;
extern pthread_t global_tid_indexop;

int init_motr(void) {
  s3_log(S3_LOG_INFO, "", "Entering!\n");
  int rc;
  S3Option *option_instance = S3Option::get_instance();
  /* MOTR_DEFAULT_EP, MOTR_DEFAULT_HA_ADDR*/
  motr_conf.mc_is_oostore = option_instance->get_motr_is_oostore();
  motr_conf.mc_is_read_verify = option_instance->get_motr_is_read_verify();
  motr_conf.mc_is_addb_init = FLAGS_addb;
  motr_conf.mc_local_addr = option_instance->get_motr_local_addr().c_str();
  motr_conf.mc_ha_addr = option_instance->get_motr_ha_addr().c_str();
  motr_conf.mc_profile = option_instance->get_motr_prof().c_str();
  motr_conf.mc_process_fid = option_instance->get_motr_process_fid().c_str();
  motr_conf.mc_tm_recv_queue_min_len =
      option_instance->get_motr_tm_recv_queue_min_len();
  motr_conf.mc_max_rpc_msg_size = option_instance->get_motr_max_rpc_msg_size();
  motr_conf.mc_layout_id = option_instance->get_motr_layout_id();

  int idx_service_id = option_instance->get_motr_idx_service_id();
  switch (idx_service_id) {
    case 0:
#if 0
      /* To be replaced in case of cassandra */
      motr_conf.mc_idx_service_id        = M0_IDX_MOCK;
      motr_conf.mc_idx_service_conf      = (void *)motr_indices;
#endif
      s3_log(S3_LOG_FATAL, "", "KVS Index service Id [%d] not supported\n",
             idx_service_id);
      return -1;
      break;

    case 1:
      s3_log(S3_LOG_INFO, "", "KVS Index service Id M0_IDX_DIX selected\n");
      motr_conf.mc_idx_service_id = M0_IDX_DIX;
      dix_conf.kc_create_meta = false;
      motr_conf.mc_idx_service_conf = &dix_conf;
      break;

    case 2:
      s3_log(S3_LOG_INFO, "", "KVS Index service Id M0_IDX_CASS selected\n");
      cass_conf.cc_cluster_ep = const_cast<char *>(
          option_instance->get_motr_cass_cluster_ep().c_str());
      cass_conf.cc_keyspace =
          const_cast<char *>(option_instance->get_motr_cass_keyspace().c_str());
      cass_conf.cc_max_column_family_num =
          option_instance->get_motr_cass_max_column_family_num();
      motr_conf.mc_idx_service_id = M0_IDX_CASS;
      motr_conf.mc_idx_service_conf = &cass_conf;
      break;

    default:
      s3_log(S3_LOG_FATAL, "", "KVS Index service Id [%d] not supported\n",
             idx_service_id);
      return -1;
  }

  /* Motr client instance */
  rc = m0_client_init(&motr_instance, &motr_conf, true);

  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "", "Failed to initilise Motr Client: %d\n", rc);
    return rc;
  }

  /* And finally, motr client root scope */
  m0_container_init(&motr_container, NULL, &M0_UBER_REALM, motr_instance);
  rc = motr_container.co_realm.re_entity.en_sm.sm_rc;

  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "", "Failed to open uber scope\n");
    fini_motr();
    return rc;
  }

  motr_uber_realm = motr_container.co_realm;
  rc = m0_ufid_init(motr_instance, &s3_ufid_generator);
  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "", "Failed to initialize ufid generator: %d\n", rc);
    return rc;
  }

  return 0;
}

void fini_motr(void) {
  s3_log(S3_LOG_INFO, "", "Entering!\n");
  m0_ufid_fini(&s3_ufid_generator);
  m0_client_fini(motr_instance, true);
  s3_log(S3_LOG_INFO, "", "Exiting\n");
}

int create_new_instance_id(struct m0_uint128 *ufid) {
  // Unique OID generation by motr.

  std::unique_ptr<MotrAPI> s3_motr_api =
      std::unique_ptr<ConcreteMotrAPI>(new ConcreteMotrAPI());

  int rc;

  if (ufid == NULL) {
    s3_log(S3_LOG_ERROR, "", "Invalid argument, ufid pointer is NULL");
    return -EINVAL;
  }

  rc = s3_motr_api->m0_h_ufid_next(ufid);
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, "", "Failed to generate UFID\n");
    s3_iem(LOG_ALERT, S3_IEM_MOTR_CONN_FAIL, S3_IEM_MOTR_CONN_FAIL_STR,
           S3_IEM_MOTR_CONN_FAIL_JSON);
    return rc;
  }
  s3_log(S3_LOG_INFO, "",
         "Instance Id "
         "%" SCNx64 " : %" SCNx64 "\n",
         ufid->u_hi, ufid->u_lo);
  return rc;
}

void teardown_motr_op(struct m0_op *op) { teardown_motr_cancel_wait_op(op, 0); }

void teardown_motr_cancel_wait_op(struct m0_op *op, int sec) {
  if (!op) return;

  if (M0_OS_LAUNCHED == op->op_sm.sm_state) {
    m0_op_cancel(&op, 1);
    m0_op_wait(op, M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
               sec < 0 ? M0_TIME_NEVER : !sec ? M0_TIME_IMMEDIATELY
                                              : m0_time_from_now(sec, 0));
  }
  switch (op->op_sm.sm_state) {
    case M0_OS_INITIALISED:
    case M0_OS_STABLE:
    case M0_OS_FAILED:
      m0_op_fini(op);
  }
  if (M0_OS_UNINITIALISED == op->op_sm.sm_state) {
    m0_op_free(op);
  }
}

void *thread_teardown_obj_ops(void *arg) {
  auto *op_ctx = static_cast<struct s3_motr_op_context *>(arg);
  for (size_t i = 0; i < op_ctx->op_count; i++) {
    if (op_ctx->ops[i] != NULL) {
      teardown_motr_cancel_wait_op(op_ctx->ops[i]);
    }
  }
  return NULL;
}

void *teardown_obj_ops(void *) {
  pthread_t tids[MAX_THREAD];
  if (global_motr_object_ops_list.size() != 0) {
    size_t count = 0;
    for (auto op_ctx : global_motr_object_ops_list) {
      int rc = pthread_create(&tids[count++], NULL, &thread_teardown_obj_ops,
                              (void *)op_ctx);
      if (rc != 0) {
        s3_log(S3_LOG_FATAL, "", "Failed to create pthread\n");
      }
      if (count == MAX_THREAD) {
        for (size_t i = 0; i < MAX_THREAD; i++) {
          pthread_join(tids[i], NULL);
        }
        count = 0;
      }
    }
    for (size_t i = 0; i < count; i++) {
      pthread_join(tids[i], NULL);
    }
    global_motr_object_ops_list.clear();
  }
  s3_log(S3_LOG_INFO, "", "Calling m0_obj_fini...\n");
  for (auto obj_ctx : global_motr_obj) {
    for (size_t i = 0; i < obj_ctx->n_initialized_contexts; i++) {
      if (obj_ctx->objs[i].ob_entity.en_sm.sm_state != 0) {
        m0_obj_fini(&obj_ctx->objs[i]);
      }
    }
  }
  global_motr_obj.clear();
  return NULL;
}

void *teardown_index_ops(void *) {
  s3_log(S3_LOG_INFO, "", "Calling teardown of index operations...\n");
  for (auto idx_op_ctx : global_motr_idx_ops_list) {
    for (size_t i = 0; i < idx_op_ctx->op_count; i++) {
      if (idx_op_ctx->ops[i] != NULL) {
        teardown_motr_cancel_wait_op(idx_op_ctx->ops[i]);
      }
    }
    if (idx_op_ctx->sync_op != NULL) {
      teardown_motr_cancel_wait_op(idx_op_ctx->sync_op);
    }
  }
  global_motr_idx_ops_list.clear();
  s3_log(S3_LOG_INFO, "", "Calling m0_idx_fini...\n");
  for (auto idx_ctx : global_motr_idx) {
    for (size_t i = 0; i < idx_ctx->n_initialized_contexts; i++) {
      if (idx_ctx->idx[i].in_entity.en_sm.sm_state != 0) {
        m0_idx_fini(&idx_ctx->idx[i]);
      }
    }
  }
  global_motr_idx.clear();
  return NULL;
}

// This function being called during shutdown to teardown
// various index and object operations in progress
void global_motr_teardown() {
  int rc;
  s3_log(S3_LOG_INFO, "", "Calling teardown of object operations...\n");
  rc = pthread_create(&global_tid_objop, NULL, &teardown_obj_ops, NULL);
  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "", "Failed to create pthread\n");
  }

  rc = pthread_create(&global_tid_indexop, NULL, &teardown_index_ops, NULL);
  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "", "Failed to create pthread\n");
  }
}

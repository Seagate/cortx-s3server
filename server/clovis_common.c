#include "clovis_helpers.h"
#include <stdio.h>

static struct m0                  mero;
static struct m0_clovis          *clovis_instance = NULL;
struct m0_clovis_container clovis_container;
struct m0_clovis_scope     clovis_uber_scope;
static struct m0_clovis_config    clovis_conf;

static struct m0_idx_cass_config cass_conf;

const char *clovis_indices = "./indices";

// extern struct m0_addb_ctx m0_clovis_addb_ctx;

int init_clovis(const char *clovis_local_addr, const char *clovis_confd_addr, const char *clovis_prof)
{
  int rc;

  /* CLOVIS_DEFAULT_EP, CLOVIS_DEFAULT_HA_ADDR*/
  clovis_conf.cc_is_oostore            = false;
  clovis_conf.cc_is_read_verify        = false;
  clovis_conf.cc_local_addr            = clovis_local_addr;
  clovis_conf.cc_ha_addr               = CLOVIS_DEFAULT_HA_ADDR;
  clovis_conf.cc_confd                 = clovis_confd_addr;
  clovis_conf.cc_profile               = clovis_prof;
  clovis_conf.cc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
  clovis_conf.cc_max_rpc_msg_size      = M0_RPC_DEF_MAX_RPC_MSG_SIZE;

  /* To be replaced in case of cassandra */
  clovis_conf.cc_idx_service_id        = M0_CLOVIS_IDX_MOCK;
  clovis_conf.cc_idx_service_conf      = (void *)clovis_indices;

#if 0
  cass_conf.cc_cluster_ep              = "127.0.0.1";
  cass_conf.cc_keyspace                = "clovis_index_keyspace";
  cass_conf.cc_max_column_family_num   = 1;
  clovis_conf.cc_idx_service_id        = M0_CLOVIS_IDX_CASS;
  clovis_conf.cc_idx_service_conf      = &cass_conf;
#endif

  /* mero initilisation */
  m0_init(&mero);

  /* Clovis instance */
  rc = m0_clovis_init(&clovis_instance, &clovis_conf);

  if (rc != 0) {
    printf("Failed to initilise Clovis: %d\n", rc);
    goto err_exit;
  }

  /* And finally, clovis root scope */
  m0_clovis_container_init(&clovis_container,
         NULL, &M0_CLOVIS_UBER_SCOPE,
         clovis_instance);
  rc = clovis_container.co_scope.sc_entity.en_sm.sm_rc;

  if (rc != 0) {
    printf("Failed to open uber scope\n");
    goto err_exit;
  }

  clovis_uber_scope = clovis_container.co_scope;
  return 0;

err_exit:
  m0_fini();
  return rc;
}

void fini_clovis(void)
{
  m0_clovis_fini(&clovis_instance);
  m0_fini();
}

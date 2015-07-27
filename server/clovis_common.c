#include "clovis_helpers.h"
#include <stdio.h>

static struct m0                  mero;
static struct m0_clovis          *clovis_instance = NULL;
static struct m0_clovis_container clovis_container;
struct m0_clovis_scope     clovis_uber_scope;
static struct m0_clovis_config    clovis_conf; 

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
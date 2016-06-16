/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include "clovis_helpers.h"
#include "s3_log.h"
#include <stdio.h>

static struct m0_clovis          *clovis_instance = NULL;
struct m0_clovis_container clovis_container;
struct m0_clovis_realm     clovis_uber_realm;
static struct m0_clovis_config    clovis_conf;

static struct m0_idx_cass_config cass_conf;

const char *clovis_indices = "./indices";

// extern struct m0_addb_ctx m0_clovis_addb_ctx;

int init_clovis(const char *clovis_local_addr, const char *clovis_ha_addr, const char *clovis_confd_addr, const char *clovis_prof, short layout_id)
{
  s3_log(S3_LOG_INFO, "Entering!\n");
  int rc;
  /* CLOVIS_DEFAULT_EP, CLOVIS_DEFAULT_HA_ADDR*/
  clovis_conf.cc_is_oostore            = false;
  clovis_conf.cc_is_read_verify        = false;
  clovis_conf.cc_local_addr            = clovis_local_addr;
  clovis_conf.cc_ha_addr               = clovis_ha_addr;
  clovis_conf.cc_confd                 = clovis_confd_addr;
  clovis_conf.cc_profile               = clovis_prof;
  clovis_conf.cc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
  clovis_conf.cc_max_rpc_msg_size      = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
  clovis_conf.cc_layout_id = layout_id;
#if 0
  /* To be replaced in case of cassandra */
  clovis_conf.cc_idx_service_id        = M0_CLOVIS_IDX_MOCK;
  clovis_conf.cc_idx_service_conf      = (void *)clovis_indices;
#endif

  cass_conf.cc_cluster_ep              = "127.0.0.1";
  cass_conf.cc_keyspace                = "clovis_index_keyspace";
  cass_conf.cc_max_column_family_num   = 1;
  clovis_conf.cc_idx_service_id        = M0_CLOVIS_IDX_CASS;
  clovis_conf.cc_idx_service_conf      = &cass_conf;


  /* Clovis instance */
  rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);

  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "Failed to initilise Clovis: %d\n", rc);
    return rc;
  }

  /* And finally, clovis root scope */
  m0_clovis_container_init(&clovis_container,
         NULL, &M0_CLOVIS_UBER_REALM,
         clovis_instance);
  rc = clovis_container.co_realm.re_entity.en_sm.sm_rc;

  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "Failed to open uber scope\n");
    fini_clovis();
    return rc;
  }

  clovis_uber_realm = clovis_container.co_realm;
  return 0;
}

void fini_clovis(void)
{
  s3_log(S3_LOG_INFO, "Entering!\n");
  m0_clovis_fini(&clovis_instance, true);
}

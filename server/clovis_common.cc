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

#include <stdio.h>
#include "clovis_helpers.h"
#include "s3_log.h"
#include "s3_option.h"

static struct m0_clovis *clovis_instance = NULL;
struct m0_clovis_container clovis_container;
struct m0_clovis_realm clovis_uber_realm;
static struct m0_clovis_config clovis_conf;

static struct m0_idx_cass_config cass_conf;

const char *clovis_indices = "./indices";

// extern struct m0_addb_ctx m0_clovis_addb_ctx;

int init_clovis(void) {
  s3_log(S3_LOG_INFO, "Entering!\n");
  int rc;
  S3Option *option_instance = S3Option::get_instance();
  /* CLOVIS_DEFAULT_EP, CLOVIS_DEFAULT_HA_ADDR*/
  clovis_conf.cc_is_oostore = option_instance->get_clovis_is_oostore();
  clovis_conf.cc_is_read_verify = option_instance->get_clovis_is_read_verify();
  clovis_conf.cc_local_addr = option_instance->get_clovis_local_addr().c_str();
  clovis_conf.cc_ha_addr = option_instance->get_clovis_ha_addr().c_str();
  clovis_conf.cc_confd = option_instance->get_clovis_confd_addr().c_str();
  clovis_conf.cc_profile = option_instance->get_clovis_prof().c_str();
  clovis_conf.cc_process_fid =
      option_instance->get_clovis_process_fid().c_str();
  clovis_conf.cc_tm_recv_queue_min_len =
      option_instance->get_clovis_tm_recv_queue_min_len();
  clovis_conf.cc_max_rpc_msg_size =
      option_instance->get_clovis_max_rpc_msg_size();
  clovis_conf.cc_layout_id = option_instance->get_clovis_layout_id();

  int idx_service_id = option_instance->get_clovis_idx_service_id();
  switch (idx_service_id) {
    case 0:
#if 0
      /* To be replaced in case of cassandra */
      clovis_conf.cc_idx_service_id        = M0_CLOVIS_IDX_MOCK;
      clovis_conf.cc_idx_service_conf      = (void *)clovis_indices;
#endif
      s3_log(S3_LOG_FATAL, "KVS Index service Id [%d] not supported\n",
             idx_service_id);
      return -1;
      break;

    case 1:
      s3_log(S3_LOG_FATAL, "KVS Index service Id [%d] not supported\n",
             idx_service_id);
      return -1;
      break;

    case 2:
      cass_conf.cc_cluster_ep = const_cast<char *>(
          option_instance->get_clovis_cass_cluster_ep().c_str());
      cass_conf.cc_keyspace = const_cast<char *>(
          option_instance->get_clovis_cass_keyspace().c_str());
      cass_conf.cc_max_column_family_num =
          option_instance->get_clovis_cass_max_column_family_num();
      clovis_conf.cc_idx_service_id = M0_CLOVIS_IDX_CASS;
      clovis_conf.cc_idx_service_conf = &cass_conf;
      break;

    default:
      s3_log(S3_LOG_FATAL, "KVS Index service Id [%d] not supported\n",
             idx_service_id);
      return -1;
  }

  /* Clovis instance */
  rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);

  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "Failed to initilise Clovis: %d\n", rc);
    return rc;
  }

  /* And finally, clovis root scope */
  m0_clovis_container_init(&clovis_container, NULL, &M0_CLOVIS_UBER_REALM,
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

void fini_clovis(void) {
  s3_log(S3_LOG_INFO, "Entering!\n");
  m0_clovis_fini(&clovis_instance, true);
}

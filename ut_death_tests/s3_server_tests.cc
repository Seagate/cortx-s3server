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
 * Original creation date: 1-Oct-2015
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "clovis_helpers.h"
#include "s3_error_messages.h"
#include "s3_log.h"
#include "s3_memory_pool.h"
#include "s3_option.h"
#include "s3_stats.h"

// Some declarations from s3server that are required to get compiled.
// TODO - Remove such globals by implementing config file.
// S3 Auth service
const char *auth_ip_addr = "127.0.0.1";
uint16_t auth_port = 8095;
extern int s3log_level;
struct m0_uint128 root_account_user_index_oid;
S3Option *g_option_instance = NULL;
extern S3Stats *g_stats_instance;
evbase_t *global_evbase_handle;

/* global Memory Pool for read from clovis */
MemoryPoolHandle g_clovis_read_mem_pool_handle;

static void _init_log() {
  s3log_level = S3_LOG_FATAL;
  FLAGS_log_dir = "./";
  FLAGS_minloglevel = (s3log_level == S3_LOG_DEBUG) ? S3_LOG_INFO : s3log_level;
  google::InitGoogleLogging("s3ut");
}

static void _fini_log() {
  google::FlushLogFiles(google::GLOG_INFO);
  google::ShutdownGoogleLogging();
}

int main(int argc, char **argv) {
  int rc;

  g_option_instance = S3Option::get_instance();
  g_stats_instance = S3Stats::get_instance();
  _init_log();

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  S3ErrorMessages::init_messages("resources/s3_error_messages.json");

  size_t libevent_pool_buffer_size =
      g_option_instance->get_libevent_pool_buffer_size();
  event_use_mempool(libevent_pool_buffer_size, libevent_pool_buffer_size * 100,
                    libevent_pool_buffer_size * 100,
                    libevent_pool_buffer_size * 500, CREATE_ALIGNED_MEMORY);

  size_t clovis_unit_size = g_option_instance->get_clovis_unit_size();
  mempool_create(clovis_unit_size, clovis_unit_size * 100,
                 clovis_unit_size * 100, clovis_unit_size * 500,
                 CREATE_ALIGNED_MEMORY, &g_clovis_read_mem_pool_handle);

  rc = RUN_ALL_TESTS();

  mempool_destroy(&g_clovis_read_mem_pool_handle);

  _fini_log();

  if (g_stats_instance) {
    S3Stats::delete_instance();
  }
  if (g_option_instance) {
    S3Option::destroy_instance();
  }

  return rc;
}

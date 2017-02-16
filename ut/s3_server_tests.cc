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

extern "C" {
#include "clovis/clovis.h"
#include "lib/uuid.h"  // for m0_node_uuid_string_set();
#include "mero/init.h"
#include "module/instance.h"
}

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

/* global Memory Pool for read from clovis */
MemoryPoolHandle g_clovis_read_mem_pool_handle;
struct m0 instance;

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

static void _init_option_and_instance() {
  g_option_instance = S3Option::get_instance();
  g_option_instance->set_stats_whitelist_filename(
      "s3stats-whitelist-test.yaml");
  g_stats_instance = S3Stats::get_instance();
}

static void _cleanup_option_and_instance() {
  if (g_stats_instance) {
    S3Stats::delete_instance();
  }
  if (g_option_instance) {
    S3Option::destroy_instance();
  }
}

static int clovis_ut_init() {
  int rc;
  // Mero lib initialization routines
  m0_node_uuid_string_set(NULL);
  rc = m0_init(&instance);
  if (rc != 0) {
    EXPECT_EQ(rc, 0) << "Mero lib initialization failed";
  }
  return rc;
}

static void clovis_ut_fini() {
  // Mero lib cleanup
  m0_fini();
}

static int mempool_init() {
  int rc;

  size_t libevent_pool_buffer_size =
      g_option_instance->get_libevent_pool_buffer_size();
  rc = event_use_mempool(
      libevent_pool_buffer_size, libevent_pool_buffer_size * 100,
      libevent_pool_buffer_size * 100, libevent_pool_buffer_size * 500,
      CREATE_ALIGNED_MEMORY);
  if (rc != 0) return rc;

  size_t clovis_unit_size = g_option_instance->get_clovis_unit_size();
  rc = mempool_create(clovis_unit_size, clovis_unit_size * 100,
                      clovis_unit_size * 100, clovis_unit_size * 500,
                      CREATE_ALIGNED_MEMORY, &g_clovis_read_mem_pool_handle);

  return rc;
}

static void mempool_fini() {
  mempool_destroy(&g_clovis_read_mem_pool_handle);
  event_destroy_mempool();
}

int main(int argc, char **argv) {
  int rc;

  _init_log();
  _init_option_and_instance();

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  S3ErrorMessages::init_messages("resources/s3_error_messages.json");

  // Clovis Initialization
  rc = clovis_ut_init();
  if (rc != 0) {
    _cleanup_option_and_instance();
    _fini_log();
    return rc;
  }

  // Mempool Initialization
  rc = mempool_init();
  if (rc != 0) {
    clovis_ut_fini();
    _cleanup_option_and_instance();
    _fini_log();
    return rc;
  }

  rc = RUN_ALL_TESTS();

  mempool_fini();
  clovis_ut_fini();
  _cleanup_option_and_instance();
  _fini_log();

  return rc;
}

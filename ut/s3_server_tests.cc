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

#include <glog/logging.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

extern "C" {
#include "motr/client.h"
#include "lib/uuid.h"  // for m0_node_uuid_string_set();
#include "motr/init.h"
#include "module/instance.h"
}

#include "motr_helpers.h"
#include "s3_motr_layout.h"
#include "s3_error_messages.h"
#include "s3_log.h"
#include "s3_mem_pool_manager.h"
#include "s3_option.h"
#include "s3_stats.h"

// Some declarations from s3server that are required to get compiled.
// TODO - Remove such globals by implementing config file.
// S3 Auth service
const char *auth_ip_addr = "127.0.0.1";
uint16_t auth_port = 8095;
extern int s3log_level;
struct m0_uint128 global_bucket_list_index_oid;
struct m0_uint128 replica_global_bucket_list_index_oid;
struct m0_uint128 bucket_metadata_list_index_oid;
struct m0_uint128 replica_bucket_metadata_list_index_oid;
struct m0_uint128 global_probable_dead_object_list_index_oid;
struct m0_uint128 global_instance_id;
S3Option *g_option_instance = NULL;
evhtp_ssl_ctx_t *g_ssl_auth_ctx;
extern S3Stats *g_stats_instance;
pthread_t global_tid_indexop;
pthread_t global_tid_objop;
int global_shutdown_in_progress;
int shutdown_motr_teardown_called;
std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
std::set<struct s3_motr_idx_context *> global_motr_idx;
std::set<struct s3_motr_obj_context *> global_motr_obj;

struct m0 instance;

static void _init_log() {
  s3log_level = S3_LOG_FATAL;
  FLAGS_log_dir = "./";

  switch (s3log_level) {
    case S3_LOG_WARN:
      FLAGS_minloglevel = google::GLOG_WARNING;
      break;
    case S3_LOG_ERROR:
      FLAGS_minloglevel = google::GLOG_ERROR;
      break;
    case S3_LOG_FATAL:
      FLAGS_minloglevel = google::GLOG_FATAL;
      break;
    default:
      FLAGS_minloglevel = google::GLOG_INFO;
  }
  google::InitGoogleLogging("s3ut");
}

static void _fini_log() {
  google::FlushLogFiles(google::GLOG_INFO);
  google::ShutdownGoogleLogging();
}

static int _init_option_and_instance() {
  g_option_instance = S3Option::get_instance();
  g_option_instance->set_option_file("s3config-test.yaml");
  bool force_override_from_config = true;
  if (!g_option_instance->load_all_sections(force_override_from_config)) {
    return -1;
  }

  g_option_instance->set_stats_allowlist_filename(
      "s3stats-allowlist-test.yaml");
  g_stats_instance = S3Stats::get_instance();
  g_option_instance->dump_options();
  S3MotrLayoutMap::get_instance()->load_layout_recommendations(
      g_option_instance->get_layout_recommendation_file());

  return 0;
}

static void _cleanup_option_and_instance() {
  if (g_stats_instance) {
    S3Stats::delete_instance();
  }
  if (g_option_instance) {
    S3Option::destroy_instance();
  }
  S3MotrLayoutMap::destroy_instance();
}

static int motr_ut_init() {
  int rc;
  // Motr lib initialization routines
  m0_node_uuid_string_set(NULL);
  rc = m0_init(&instance);
  if (rc != 0) {
    EXPECT_EQ(rc, 0) << "Motr lib initialization failed";
  }
  return rc;
}

static void motr_ut_fini() {
  // Motr lib cleanup
  m0_fini();
}

static int mempool_init() {
  int rc;

  size_t libevent_pool_buffer_size =
      g_option_instance->get_libevent_pool_buffer_size();

  int motr_read_mempool_flags = CREATE_ALIGNED_MEMORY;
  if (g_option_instance->get_motr_read_mempool_zeroed_buffer()) {
    motr_read_mempool_flags = motr_read_mempool_flags | ZEROED_BUFFER;
  }

  int libevent_mempool_flags = CREATE_ALIGNED_MEMORY;
  if (g_option_instance->get_libevent_mempool_zeroed_buffer()) {
    libevent_mempool_flags = libevent_mempool_flags | ZEROED_BUFFER;
  }

  rc = event_use_mempool(
      libevent_pool_buffer_size, libevent_pool_buffer_size * 100,
      libevent_pool_buffer_size * 100, libevent_pool_buffer_size * 1000,
      libevent_mempool_flags);
  if (rc != 0) return rc;

  rc = S3MempoolManager::create_pool(
      g_option_instance->get_motr_read_pool_max_threshold(),
      g_option_instance->get_motr_unit_sizes_for_mem_pool(),
      g_option_instance->get_motr_read_pool_initial_buffer_count(),
      g_option_instance->get_motr_read_pool_expandable_count(),
      motr_read_mempool_flags);

  return rc;
}

static void mempool_fini() {
  S3MempoolManager::destroy_instance();
  event_destroy_mempool();
}

static bool init_auth_ssl() {
  const char *cert_file = g_option_instance->get_iam_cert_file();
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  g_ssl_auth_ctx = SSL_CTX_new(SSLv23_method());
  if (!SSL_CTX_load_verify_locations(g_ssl_auth_ctx, cert_file, NULL)) {
    s3_log(S3_LOG_ERROR, "", "SSL_CTX_load_verify_locations\n");
    return false;
  }
  SSL_CTX_set_verify(g_ssl_auth_ctx, SSL_VERIFY_PEER, NULL);
  SSL_CTX_set_verify_depth(g_ssl_auth_ctx, 1);
  return true;
}

int main(int argc, char **argv) {
  int rc;

  _init_log();
  rc = _init_option_and_instance();
  if (rc != 0) {
    _cleanup_option_and_instance();
    _fini_log();
    return rc;
  }

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  S3ErrorMessages::init_messages("resources/s3_error_messages.json");

  // Motr Initialization
  rc = motr_ut_init();
  if (rc != 0) {
    _cleanup_option_and_instance();
    _fini_log();
    return rc;
  }

  // Mempool Initialization
  rc = mempool_init();
  if (rc != 0) {
    motr_ut_fini();
    _cleanup_option_and_instance();
    _fini_log();
    return rc;
  }

  // SSL initialization
  if (init_auth_ssl() != true) {
    mempool_fini();
    motr_ut_fini();
    _cleanup_option_and_instance();
    _fini_log();
    return rc;
  }

  rc = RUN_ALL_TESTS();

  mempool_fini();
  motr_ut_fini();
  _cleanup_option_and_instance();
  _fini_log();

  return rc;
}

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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "motr_helpers.h"
#include "s3_motr_layout.h"
#include "s3_error_messages.h"
#include "s3_log.h"
#include "s3_mem_pool_manager.h"
#include "s3_option.h"
#include "s3_stats.h"
#include "s3_audit_info_logger.h"

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
pthread_t global_tid_indexop;
pthread_t global_tid_objop;
S3Option *g_option_instance = NULL;
evhtp_ssl_ctx_t *g_ssl_auth_ctx = NULL;
extern S3Stats *g_stats_instance;
evbase_t *global_evbase_handle = nullptr;
int global_shutdown_in_progress;
int shutdown_motr_teardown_called;
std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
std::set<struct s3_motr_idx_context *> global_motr_idx;
std::set<struct s3_motr_obj_context *> global_motr_obj;

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

  if (S3AuditInfoLogger::init() != 0) {
    s3_log(S3_LOG_FATAL, "", "Couldn't init audit logger!");
    return -1;
  }

  S3ErrorMessages::init_messages("resources/s3_error_messages.json");

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

  event_use_mempool(libevent_pool_buffer_size, libevent_pool_buffer_size * 100,
                    libevent_pool_buffer_size * 100,
                    libevent_pool_buffer_size * 500, libevent_mempool_flags);

  rc = S3MempoolManager::create_pool(
      g_option_instance->get_motr_read_pool_max_threshold(),
      g_option_instance->get_motr_unit_sizes_for_mem_pool(),
      g_option_instance->get_motr_read_pool_initial_buffer_count(),
      g_option_instance->get_motr_read_pool_expandable_count(),
      motr_read_mempool_flags);

  rc = RUN_ALL_TESTS();

  S3MempoolManager::destroy_instance();

  _fini_log();
  S3AuditInfoLogger::finalize();

  if (g_stats_instance) {
    S3Stats::delete_instance();
  }
  if (g_option_instance) {
    S3Option::destroy_instance();
  }

  return rc;
}

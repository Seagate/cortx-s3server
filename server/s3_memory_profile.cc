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

#include <event2/event.h>

#include "s3_motr_layout.h"
#include "s3_log.h"
#include "s3_memory_pool.h"
#include "s3_memory_profile.h"
#include "s3_option.h"

extern S3Option* g_option_instance;

size_t S3MemoryProfile::memory_per_put_request(int layout_id) {
  return g_option_instance->get_motr_write_payload_size(layout_id) *
         g_option_instance->get_read_ahead_multiple();
}

bool S3MemoryProfile::we_have_enough_memory_for_put_obj(int layout_id) {
#ifdef S3_GOOGLE_TEST
  return true;
#endif
  size_t free_space_in_libevent_mempool = 0;
  event_mempool_free_space(&free_space_in_libevent_mempool);
  s3_log(S3_LOG_DEBUG, "", "free_space_in_libevent_mempool = %zu\n",
         free_space_in_libevent_mempool);

  size_t min_mem_for_put_obj = memory_per_put_request(layout_id);

  s3_log(S3_LOG_DEBUG, "", "min_mem_for_put_obj = %zu\n", min_mem_for_put_obj);

  return (free_space_in_libevent_mempool > min_mem_for_put_obj);
}

bool S3MemoryProfile::free_memory_in_pool_above_threshold_limits() {
#ifdef S3_GOOGLE_TEST
  return true;
#endif
  size_t free_space_in_libevent_mempool = 0;
  // libevent uses ev_buffers for some internal tasks so
  // memory pool can be exhausted under high load
  const auto libevent_pool_reserve_size =
      g_option_instance->get_libevent_pool_reserve_size();
  event_mempool_free_space(&free_space_in_libevent_mempool);
  s3_log(S3_LOG_DEBUG, "", "free_space_in_libevent_mempool = %zu\n",
         free_space_in_libevent_mempool);
  if (free_space_in_libevent_mempool < libevent_pool_reserve_size) {
    s3_log(
        S3_LOG_WARN, "",
        "Free space in libevent mempool is less than "
        "S3_LIBEVENT_POOL_RESERVE_SIZE defined in config file (s3config.yaml)");
    struct pool_info poolinfo;
    int rc = event_mempool_getinfo(&poolinfo);
    if (rc != 0) {
      s3_log(S3_LOG_FATAL, "", "Issue with memory pool!\n");
    } else {
      s3_log(S3_LOG_WARN, "",
             "mempool info: mempool_item_size = %zu "
             "free_bufs_in_pool = %d "
             "number_of_bufs_shared = %d "
             "total_bufs_allocated_by_pool = %d\n",
             poolinfo.mempool_item_size, poolinfo.free_bufs_in_pool,
             poolinfo.number_of_bufs_shared,
             poolinfo.total_bufs_allocated_by_pool);
    }
    return false;
  }
  const auto libevent_pool_max_threshold =
      g_option_instance->get_libevent_pool_max_threshold();
  const auto libevent_pool_reserve_percent =
      g_option_instance->get_libevent_pool_reserve_percent();
  if (libevent_pool_reserve_percent / 100.0 * libevent_pool_max_threshold >
      free_space_in_libevent_mempool) {
    s3_log(S3_LOG_WARN, "",
           "Free space (in percent) in libevent mempool is less than "
           "S3_LIBEVENT_POOL_RESERVE_PERCENT defined in config file "
           "(s3config.yaml)");

    struct pool_info poolinfo;
    int rc = event_mempool_getinfo(&poolinfo);
    if (rc != 0) {
      s3_log(S3_LOG_FATAL, "", "Issue with memory pool!\n");
    } else {
      s3_log(S3_LOG_WARN, "",
             "mempool info: mempool_item_size = %zu "
             "free_bufs_in_pool = %d "
             "number_of_bufs_shared = %d "
             "total_bufs_allocated_by_pool = %d\n",
             poolinfo.mempool_item_size, poolinfo.free_bufs_in_pool,
             poolinfo.number_of_bufs_shared,
             poolinfo.total_bufs_allocated_by_pool);
    }
    return false;
  }
  return true;
}


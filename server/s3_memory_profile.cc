/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original creation date: 6-Feb-2017
 */

#include <cassert>
#include <event2/event.h>

#include "s3_clovis_layout.h"
#include "s3_log.h"
#include "s3_memory_pool.h"
#include "s3_memory_profile.h"
#include "s3_option.h"

extern S3Option* g_option_instance;

S3MemoryProfile::S3MemoryProfile() {
  assert(g_option_instance != nullptr);

  libevent_pool_max_threshold =
      g_option_instance->get_libevent_pool_max_threshold();
  libevent_pool_reserve_size =
      g_option_instance->get_libevent_pool_reserve_size();

  const auto libevent_pool_reserve_in_percent =
      g_option_instance->get_libevent_pool_reserve_percent() / 100.0 *
      libevent_pool_max_threshold;

  if (libevent_pool_reserve_in_percent > libevent_pool_reserve_size) {
    libevent_pool_reserve_size =
        static_cast<size_t>(libevent_pool_reserve_in_percent);
  }
  read_ahead_pool_usage_ratio =
      g_option_instance->get_read_ahead_pool_usage_ratio();
  read_ahead_pool_reserve = g_option_instance->get_read_ahead_pool_reserve();
}

S3MemoryProfile::S3MemoryProfile(size_t libevent_pool_max_threshold_,
                                 size_t libevent_pool_reserve_size_,
                                 size_t read_ahead_pool_reserve_,
                                 uint_fast8_t read_ahead_pool_usage_ratio_)
    : libevent_pool_max_threshold(libevent_pool_max_threshold_),
      libevent_pool_reserve_size(libevent_pool_reserve_size_),
      read_ahead_pool_reserve(read_ahead_pool_reserve_),
      read_ahead_pool_usage_ratio(read_ahead_pool_usage_ratio_) {}

S3MemoryProfile::~S3MemoryProfile() = default;

size_t S3MemoryProfile::memory_per_put_request(int layout_id) {
  return g_option_instance->get_clovis_write_payload_size(layout_id) *
         g_option_instance->get_read_ahead_multiple();
}

bool S3MemoryProfile::we_have_enough_memory_for_put_obj(int layout_id) {
#ifdef S3_GOOGLE_TEST
  return true;
#endif
  const size_t free_space_in_libevent_mempool = free_space_in_pool();
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
  const size_t free_space_in_libevent_mempool = free_space_in_pool();
  // libevent uses ev_buffers for some internal tasks so
  // memory pool can be exhausted under high load
  s3_log(S3_LOG_DEBUG, "", "free_space_in_libevent_mempool = %zu\n",
         free_space_in_libevent_mempool);
  if (free_space_in_libevent_mempool < libevent_pool_reserve_size) {
    s3_log(S3_LOG_WARN, "",
           "Free space in libevent mempool is less than "
           "S3_LIBEVENT_POOL_RESERVE_SIZE or S3_LIBEVENT_POOL_RESERVE_PERCENT"
           " defined in config file (s3config.yaml)");
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

/* Size of read ahead buffer of any given single request, at any moment in time
 * must not exceed RATIO * (FREE_SPACE_IN_POOL – RESERVE). RESERVE in turn
 * consists of 2 parts: LIBEVENT_POOL_RESERVE and READ_AHEAD_POOL_RESERVE.
 * LIBEVENT_POOL_RESERVE is needed because LibEvent uses the same memory
 * allocator for allocation both buffers and internal structures.
 * READ_AHEAD_POOL_RESERVE should be approximately equal to (1.5 *
 * max_count_expected_connections_per_instance * max_possible_unit_size(1M)).
 * RATIO should be less than 1.
 * When other requests consume more memory, and read ahead buffer of a request
 * crosses the threshold, reading is paused, until either read ahead buffer is
 * sent to Clovis for writing, or some memory is freed in pool, after that
 * request is un-paused and reading continues.
 */
size_t S3MemoryProfile::max_read_ahead_size() const {
  const size_t free_space_in_libevent_mempool = free_space_in_pool();

  if (free_space_in_libevent_mempool <
      libevent_pool_reserve_size + read_ahead_pool_reserve) {
    return 0;
  }
  return static_cast<size_t>(read_ahead_pool_usage_ratio / 100.0 *
                             (free_space_in_libevent_mempool -
                              libevent_pool_reserve_size -
                              read_ahead_pool_reserve));
}

size_t S3MemoryProfile::free_space_in_pool() const {
  size_t free_space_in_libevent_mempool = 0;
  event_mempool_free_space(&free_space_in_libevent_mempool);
  return free_space_in_libevent_mempool;
}

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

#pragma once

#ifndef __S3_SERVER_S3_MEM_POOL_MANAGER_H__
#define __S3_SERVER_S3_MEM_POOL_MANAGER_H__

#include <map>
#include <vector>

#include <gtest/gtest_prod.h>

#include "s3_log.h"
#include "s3_memory_pool.h"

// Pool of memory pools specifically for use of different unit_size buffers
class S3MempoolManager {
  // map<unit_size, memory_pool_for_unit_size>
  std::map<size_t, MemoryPoolHandle> pool_of_mem_pool;

  // map<unit_size, last_used_timestamp>
  std::map<size_t, size_t> last_used_timestamp;
  // Above can be used to identify to request freeing up space
  // for least likely used unit_size memory pool

  // Maximum memory threshold
  size_t total_memory_threshold;

  static S3MempoolManager* instance;

  S3MempoolManager(size_t max_mem);

  int initialize(std::vector<int> unit_sizes, int initial_buffer_count_per_pool,
                 size_t expandable_count, int flags);

  // Free up all the memory pools help by this pool
  void free_pools();

 public:
  // initially equal to total_memory_threshold
  // static so C callback passed to mempool can access this
  static size_t free_space;

  //  Return the buffer of give unit_size
  // For flags see mempool_getbuffer() in s3_memory_pool.h header
  void* get_buffer_for_unit_size(size_t unit_size);

  // Releases the give buffer back to pool, callers responsibility to use
  // proper unit_size else it will have **UNEXPECTED BEHAVIOR**
  int release_buffer_for_unit_size(void* buf, size_t unit_size);

  size_t get_free_space_for(size_t unit_size);

  // Free any mempool that has max free space, free it by half
  // Returns true if space was free'ed in any pool, false if it cannot be
  bool free_any_unused();

  // Creates a pool to support various given unit_sizes and maximum threshold
  static int create_pool(size_t max_mem, std::vector<int> unit_sizes,
                         int initial_buffer_count_per_pool,
                         size_t expandable_count, int flags = 0) {
    if (!instance) {
      instance = new S3MempoolManager(max_mem);
      S3MempoolManager::free_space = max_mem;
      int rc = instance->initialize(unit_sizes, initial_buffer_count_per_pool,
                                    expandable_count, flags);
      if (rc != 0) {
        S3MempoolManager::destroy_instance();
      }
      return rc;
    }
    return EEXIST;
  }

  static S3MempoolManager* get_instance() {
    if (!instance) {
      s3_log(S3_LOG_FATAL, "",
             "Memory pool for motr read buffers not initialized!\n");
    }
    return instance;
  }

  static void destroy_instance() {
    if (instance) {
      instance->free_pools();
      delete instance;
      instance = NULL;
      S3MempoolManager::free_space = 0;
    }
  }

  FRIEND_TEST(S3MempoolManagerTestSuite, CreateAndDestroyPoolTest);
  FRIEND_TEST(S3MempoolManagerTestSuite, CreateFailedTest);
  FRIEND_TEST(S3MempoolManagerTestSuite, CreateEXISTSTest);
  FRIEND_TEST(S3MempoolManagerTestSuite, PoolShouldGrowAndErrorOnMax);
  FRIEND_TEST(S3MempoolManagerTestSuite, PoolShouldGrowByDownsizingUnusedPool);
};

#endif

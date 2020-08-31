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

#ifndef __MOTR_FE_S3_SERVER_S3_MEMORY_POOL_H__
#define __MOTR_FE_S3_SERVER_S3_MEMORY_POOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdlib.h>

/* Pool Creation flags */
#define CREATE_ALIGNED_MEMORY 0x0001
#define ENABLE_LOCKING 0x0002
#define ZEROED_BUFFER 0x0004

#define MEMORY_ALIGNMENT 4096
#define S3_MEMPOOL_ERROR -1
#define S3_MEMPOOL_INVALID_ARG -2
#define S3_MEMPOOL_THRESHOLD_EXCEEDED -3

struct mempool;
typedef struct mempool *MemoryPoolHandle;

/* This call back is supposed to return the total space used.
   Useful when outside memory should be considered for max threshold cap.
   Outside memory could be case when multiple memory pools are used.
*/
typedef size_t (*func_mem_available_callback_type)(void);

typedef void (*func_mark_mem_used_callback_type)(size_t);
typedef void (*func_mark_mem_free_callback_type)(size_t);

#define MEMPOOL_LOG_FATAL 4
#define MEMPOOL_LOG_ERROR 3
#define MEMPOOL_LOG_WARN 2
#define MEMPOOL_LOG_INFO 1
#define MEMPOOL_LOG_DEBUG 0
typedef void (*func_log_callback_type)(int log_level, const char *);

struct pool_info {
  int flags;
  int free_bufs_in_pool;
  int number_of_bufs_shared;
  int total_bufs_allocated_by_pool;
  size_t mempool_item_size;
  size_t expandable_size;
};

/**
 * Flow:
 * 1) Create a memory pool and obtain the handle to it
 * 2) Provide handle to the memory pool APIs to manage pool
 * 3) Destroy the pool when its not needed
 *
 * Example 1: Simple Memory pool
 * 1) A simple memory pool is created using mempool_create api, user supplies
 *     initial pool size as 0, The pool initially doesn't have any items in it.
 * 2) When user asks for item using mempool_getbuffer, it allocates it by using
 *    native method(malloc/posix_memalign) as the free list is empty.
 * 3) When user is done with item, it releases the item using pool api
 *    (mempool_releasebuffer), mempool api will hook the item to the
 *    free list of the pool.
 * 4) Next time when user asks for the item, it will be given the item (which
 *    it released before) from the free list of the pool
 * 5) Consecutive release buffer operations via api mempool_releasebuffer api
 *    will preappend to list or free the item depending on the maximum allowed
 *    free list (configurable)
 *
 *
 *     mempool_create() --> mempool_getbuffer() --> mempool_releasebuffer() -->
 * mempool_destroy()
 *
 * Example 2: Memory Pool with preallocated items, dynamic expansion and with
 * cap on maximum item(memory) utilization
 * 1) Create the pool with api mempool_create, the pool will have some initial
 *    number of items as specified by user
 * 2) When user asks for item it will be allocated from the pool (free list
 *    will not be empty), whenever the freelist is empty then the pool will
 *    dynamically expand the free list size by expansion size
 *    (specified when creating pool), error will be returned in case if the
 *    memory consumed via pool crosses the threshold value(set when creating
 * pool).
 * 3) Whenever user releases the item via mempool_releasebuffer, it will hook
 *    the item to the list or free it depending on the maximum free list size
 *
 *     mempool_create() --> mempool_getbuffer() --> mempool_releasebuffer() -->
 *     mempool_destroy()
 */

/**
 * Create a memory pool object and do preallocation of free list in it, return
 * handle to it.
 * args:
 * pool_item_size (in) Size of each buffer items that this pool will manage,
 * minimum size is size of pointer
 * Note: In case of motr the minimum buffer size is 4KB
 * pool_initial_size (in) Initial size of the pool
 * pool_expansion_size (in) pool expansion size when all free memory
 * being used.
 * pool_max_threshold_size (in) maximum allowed memory utilization(Consumed by
 * app. + free list in pool)
 * when done via the pool
 * flags (in) if ENABLE_LOCKING then pool synchronization with lock
 * p_handle (out) On success pool handle is returned here
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_create(size_t pool_item_size, size_t pool_initial_size,
                   size_t pool_expansion_size, size_t pool_max_threshold_size,
                   func_log_callback_type log_callback_func, int flags,
                   MemoryPoolHandle *p_handle);

int mempool_create_with_shared_mem(
    size_t pool_item_size, size_t pool_initial_size, size_t pool_expansion_size,
    func_mem_available_callback_type mem_get_free_space_func,
    func_mark_mem_used_callback_type mem_mark_used_space_func,
    func_mark_mem_free_callback_type mem_mark_free_space_func,
    func_log_callback_type log_callback_func, int flags,
    MemoryPoolHandle *p_handle);

/**
 * Allocate some buffer memory via memory pool
 * args:
 * handle (in) handle to memory pool obtained from mempool_create
 * returns:
 * expected_buffer_size - used for assertion.
 * 0 on success, otherwise an error
 */
void *mempool_getbuffer(MemoryPoolHandle handle, size_t expected_buffer_size);

/**
 * Hook/Free the item that was allocated earlier from the memory pool
 * (memory alloc) to pool's free list, based on the pool's free list capacity
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * buf (in) item allocated earlier via mempool_getbuffer()
 * released_buffer_size - used for assertion
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_releasebuffer(MemoryPoolHandle handle, void *buf,
                          size_t released_buffer_size);

/**
 * Returns info about the memory pool
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * poolinfo (out) information about the pool
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_getinfo(MemoryPoolHandle handle, struct pool_info *poolinfo);

/**
 * Returns the space already allocated but not used in memory pool
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * free_bytes (out) Free bytes currently available in pool
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_reserved_space(MemoryPoolHandle handle, size_t *free_bytes);

/**
 * Returns space totally available in memory pool including available expansion
 * upto maximum threshold.
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * p_avail_bytes (out) Total bytes available for allocation in pool upto maximum
 * threshold.
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_available_space(MemoryPoolHandle handle, size_t *p_avail_bytes);

/**
 * Returns memory pool's buffer size
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * buffer_size (out) memory pool's buffer size
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_getbuffer_size(MemoryPoolHandle handle, size_t *buffer_size);

/**
 * Free up 'mem_to_free' space from this pool, so other pools can use.
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * mem_to_free (in) Bytes to free, MUST be multiple of pool_item_size
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_downsize(MemoryPoolHandle handle, size_t mem_to_free);

/**
 * Change the the pool's maximum threshold memory(Application consumed memory +
 * pools free memory), setting up maximum threshold will fail if it makes the
 * pool's maximum free list size 0.
 * In case if user wants to downsize the current maximum threshold value then it
 * may need to get the pool information and then set the new maximum threshold
 * value based on pool info, this is to ensure that max free list size of the
 * pool doesn't become 0 with this api.
 * args:
 * handle (in) Pool handle as returned by mempool_create
 * new_max_threshold (in) New max free size setting
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_resize(MemoryPoolHandle handle, int new_max_threshold);

/**
 * Destroy mem pool object along with all memories in its free list
 * Call this api when the application is shutting down, Api will assert in case
 * if there is memory allocation done via pool api and it being freed without
 * pool's free api
 * args:
 * p_handle (in/out) pointer to handle, on return this will be set to NULL
 * returns:
 * 0 on success, otherwise an error
 */
int mempool_destroy(MemoryPoolHandle *p_handle);

#ifdef __cplusplus
}
#endif

#endif

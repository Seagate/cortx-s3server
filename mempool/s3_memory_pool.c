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

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "s3_memory_pool.h"

struct memory_pool_element {
  struct memory_pool_element *next;
};

struct mempool {
  int flags;                 /* Buffer Bitflags */
  int free_bufs_in_pool;     /* Number of items on free list */
  int number_of_bufs_shared; /* Number of bufs shared from pool to pool user */
  int total_bufs_allocated_by_pool; /* Total buffers currently allocated by
                                         pool via native method */
  func_mem_available_callback_type
      mem_get_free_space_func; /* If this pool shares the max_memory_threshold
                                  with say other pool, this callback should
                                  return the available space which can be
                                  allocated */
  func_mark_mem_used_callback_type
      mem_mark_used_space_func; /* This is used to indicate to user of pool that
                                   new memory is allocated
                                   (posix_memalign/malloc) within pool, so user
                                   of pool can track it w.r.t max threshold. */
  func_mark_mem_free_callback_type mem_mark_free_space_func;
  func_log_callback_type log_callback_func; /* Used to log mempool messages */
  /* Whenever pool frees any memory, use this to
     indicate to user of pool that memory was
     free'd with actual free() sys call. */
  size_t alignment;            /* Memory aligment */
  size_t max_memory_threshold; /* Maximum memory that the system can have from
                                  pool */
  size_t mempool_item_size; /* Size of items managed by this pool */
  size_t expandable_size;   /* pool expansion rate when free list is empty */
  pthread_mutex_t lock;     /* lock, in case of synchronous operation */
  struct memory_pool_element *free_list; /* list of free items available for
                                            reuse */
};

/**
 * Return the number of buffers we can allocate w.r.t max threshold and
 * available memory space.
 */
static int pool_can_expand_by(struct mempool *pool) {
  size_t available_space = 0;

  if (pool == NULL) {
    return 0;
  }

  if (pool->mem_get_free_space_func) {
    available_space = pool->mem_get_free_space_func();
  } else {
    const size_t allocated_space =
        pool->total_bufs_allocated_by_pool * pool->mempool_item_size;

    if (pool->max_memory_threshold > allocated_space) {
      available_space = pool->max_memory_threshold - allocated_space;
    }
  }

  // We can expand by at least (available_space / pool->mempool_item_size)
  // buffer count
  return available_space / pool->mempool_item_size;
}

/**
 * Internal function to preallocate items to memory pool.
 * args:
 * pool (in) pointer to memory pool
 * items_count_to_allocate (in) Extra items to be allocated
 * returns:
 * 0 on success, otherwise an error
 */
int freelist_allocate(struct mempool *pool, int items_count_to_allocate) {
  int i;
  int rc = 0;
  void *buf = NULL;
  struct memory_pool_element *pool_item = NULL;
  char *log_msg_fmt =
      "mempool(%p): %s(%zu). Allocated address(%p)  rc(%d) alignment(%zu) "
      "size(%zu)";
  char log_msg[200];

  if (pool == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  for (i = 0; i < items_count_to_allocate; i++) {
    if (pool->flags & CREATE_ALIGNED_MEMORY) {
      buf = NULL;
      rc = posix_memalign((void **)&buf, pool->alignment,
                          pool->mempool_item_size);
    } else {
      buf = malloc(pool->mempool_item_size);
    }
    if (pool->log_callback_func) {
      if (pool->flags & CREATE_ALIGNED_MEMORY) {
        snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool,
                 "posix_memalign", pool->mempool_item_size, buf, rc,
                 pool->alignment, pool->mempool_item_size);
      } else {
        snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool, "malloc",
                 pool->mempool_item_size, buf, rc, pool->alignment,
                 pool->mempool_item_size);
      }
      pool->log_callback_func(MEMPOOL_LOG_DEBUG, log_msg);
    }
    if (buf == NULL || rc != 0) {
      return S3_MEMPOOL_ERROR;
    }

    /* exclude this buffer while geneating core dump*/
    madvise(buf, pool->mempool_item_size, MADV_DONTDUMP);

    if ((pool->flags & ZEROED_BUFFER) != 0) {
      memset(buf, 0, pool->mempool_item_size);
    }

    pool->total_bufs_allocated_by_pool++;
    /* Put the allocated memory into the list */
    pool_item = (struct memory_pool_element *)buf;

    pool_item->next = pool->free_list;
    pool->free_list = pool_item;
    /* memory is pre appended to list */

    /* Increase the free list count */
    pool->free_bufs_in_pool++;
    if (pool->mem_mark_used_space_func) {
      pool->mem_mark_used_space_func(pool->mempool_item_size);
    }
  }
  return 0;
}

int mempool_create(size_t pool_item_size, size_t pool_initial_size,
                   size_t pool_expansion_size, size_t pool_max_threshold_size,
                   func_log_callback_type log_callback_func, int flags,
                   MemoryPoolHandle *handle) {
  int rc;
  int bufs_to_allocate;
  struct mempool *pool = NULL;

  /* pool_max_threshold_size == 0 is possible when
     func_mem_available_callback_type is used. */
  if (pool_item_size == 0 || pool_expansion_size == 0 || handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  /* Minimum size of the pool's buffer will be sizeof pointer */
  if (pool_item_size < sizeof(struct memory_pool_element)) {
    pool_item_size = sizeof(struct memory_pool_element);
  }

  *handle = NULL;

  pool = (struct mempool *)calloc(1, sizeof(struct mempool));
  if (pool == NULL) {
    return S3_MEMPOOL_ERROR;
  }

  pool->flags |= flags;
  pool->mempool_item_size = pool_item_size;
  if (flags & CREATE_ALIGNED_MEMORY) {
    pool->alignment = MEMORY_ALIGNMENT;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    rc = pthread_mutex_init(&pool->lock, NULL);
    if (rc != 0) {
      free(pool);
      return S3_MEMPOOL_ERROR;
    }
  }

  *handle = (MemoryPoolHandle)pool;

  pool->log_callback_func = log_callback_func;
  pool->expandable_size = pool_expansion_size;
  pool->max_memory_threshold = pool_max_threshold_size;
  /* Figure out the size of free list to be preallocated from given initial pool
   * size */
  bufs_to_allocate = pool_initial_size / pool_item_size;

  /* Allocate the free list */
  if (bufs_to_allocate > 0) {
    rc = freelist_allocate(pool, bufs_to_allocate);
    if (rc != 0) {
      goto fail;
    }
  }
  return 0;

fail:
  mempool_destroy(handle);
  *handle = NULL;
  return S3_MEMPOOL_ERROR;
}

int mempool_create_with_shared_mem(
    size_t pool_item_size, size_t pool_initial_size, size_t pool_expansion_size,
    func_mem_available_callback_type mem_get_free_space_func,
    func_mark_mem_used_callback_type mem_mark_used_space_func,
    func_mark_mem_free_callback_type mem_mark_free_space_func,
    func_log_callback_type log_callback_func, int flags,
    MemoryPoolHandle *p_handle) {
  int rc = 0;
  struct mempool *pool = NULL;
  if (mem_get_free_space_func == NULL || mem_mark_used_space_func == NULL ||
      mem_mark_free_space_func == NULL || p_handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if (pool_initial_size > mem_get_free_space_func()) {
    return S3_MEMPOOL_THRESHOLD_EXCEEDED;
  }

  rc = mempool_create(pool_item_size, pool_initial_size, pool_expansion_size, 0,
                      log_callback_func, flags, p_handle);
  if (rc != 0) {
    return rc;
  }

  pool = (struct mempool *)*p_handle;

  pool->mem_get_free_space_func = mem_get_free_space_func;
  pool->mem_mark_used_space_func = mem_mark_used_space_func;
  pool->mem_mark_free_space_func = mem_mark_free_space_func;

  /* Explicitly mark used space, since mempool_create -> freelist_allocate
     dont have the function callbacks set. */
  pool->mem_mark_used_space_func(pool->total_bufs_allocated_by_pool *
                                 pool->mempool_item_size);

  return 0;
}

void *mempool_getbuffer(MemoryPoolHandle handle, size_t expected_buffer_size) {
  int rc;
  int bufs_to_allocate;
  int bufs_that_can_be_allocated = 0;
  struct memory_pool_element *pool_item = NULL;
  struct mempool *pool = (struct mempool *)handle;
  char *log_msg_fmt =
      "mempool(%p): mempool_getbuffer called for invalid "
      "expected_buffer_size(%zu), current pool manages only "
      "mempool_item_size(%zu)";
  char log_msg[300];

  if (pool == NULL) {
    return NULL;
  }

  if (pool->mempool_item_size != expected_buffer_size) {
    // This should never happen unless mempool is used wrongly.
    if (pool->log_callback_func) {
      snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool,
               expected_buffer_size, pool->mempool_item_size);
      pool->log_callback_func(MEMPOOL_LOG_FATAL, log_msg);
      return NULL;
    }
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  /* If the free list is empty then expand the pool's free list */
  if (pool->free_bufs_in_pool == 0) {
    bufs_to_allocate = pool->expandable_size / pool->mempool_item_size;
    bufs_that_can_be_allocated = pool_can_expand_by(pool);
    if (bufs_that_can_be_allocated > 0) {
      /* We can at least allocate
         min(bufs_that_can_be_allocated, bufs_to_allocate) */
      bufs_to_allocate = ((bufs_to_allocate > bufs_that_can_be_allocated)
                              ? bufs_that_can_be_allocated
                              : bufs_to_allocate);

      rc = freelist_allocate(pool, bufs_to_allocate);
      if (rc != 0) {
        if ((pool->flags & ENABLE_LOCKING) != 0) {
          pthread_mutex_unlock(&pool->lock);
        }
        return NULL;
      }
    } else {
      /* We cannot allocate any more buffers, reached max threshold */
      if ((pool->flags & ENABLE_LOCKING) != 0) {
        pthread_mutex_unlock(&pool->lock);
      }
      return NULL;
    }
  }

  /* Done with expansion of pool in case of pre allocated pools */

  /* Logic of allocation from free list */
  /* If there is an item on the pool's free list, then take that... */
  if (pool->free_list != NULL) {
    pool_item = pool->free_list;
    pool->free_list = pool_item->next;
    pool_item->next = (struct memory_pool_element *)NULL;
    pool->free_bufs_in_pool--;
  }

  if (pool_item) {
    pool->number_of_bufs_shared++;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }

  return (void *)pool_item;
}

int mempool_releasebuffer(MemoryPoolHandle handle, void *buf,
                          size_t released_buffer_size) {
  struct mempool *pool = (struct mempool *)handle;
  struct memory_pool_element *pool_item = (struct memory_pool_element *)buf;
  char *log_msg_fmt =
      "mempool(%p): mempool_releasebuffer called for invalid "
      "released_buffer_size(%zu), current pool manages only "
      "mempool_item_size(%zu)";
  char log_msg[300];

  if ((pool == NULL) || (pool_item == NULL)) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if (pool->mempool_item_size != released_buffer_size) {
    // This should never happen unless mempool is used wrongly.
    if (pool->log_callback_func) {
      snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool,
               released_buffer_size, pool->mempool_item_size);
      pool->log_callback_func(MEMPOOL_LOG_FATAL, log_msg);
      return S3_MEMPOOL_INVALID_ARG;
    }
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  /* Clean up the buffer so that we get it 'clean' when we allocate it next
   * time*/
  if ((pool->flags & ZEROED_BUFFER) != 0) {
    memset(pool_item, 0, pool->mempool_item_size);
  }

  // Add the buffer back to pool
  pool_item->next = pool->free_list;
  pool->free_list = pool_item;
  pool->free_bufs_in_pool++;
  pool_item = NULL;

  pool->number_of_bufs_shared--;

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }

  return 0;
}

int mempool_getinfo(MemoryPoolHandle handle, struct pool_info *poolinfo) {
  struct mempool *pool = (struct mempool *)handle;

  if ((pool == NULL) || (poolinfo == NULL)) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if (pool->flags & ENABLE_LOCKING) {
    pthread_mutex_lock(&pool->lock);
  }

  poolinfo->mempool_item_size = pool->mempool_item_size;
  poolinfo->free_bufs_in_pool = pool->free_bufs_in_pool;
  poolinfo->number_of_bufs_shared = pool->number_of_bufs_shared;
  poolinfo->expandable_size = pool->expandable_size;
  poolinfo->total_bufs_allocated_by_pool = pool->total_bufs_allocated_by_pool;
  poolinfo->flags = pool->flags;

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }
  return 0;
}

int mempool_reserved_space(MemoryPoolHandle handle, size_t *free_bytes) {
  struct mempool *pool = (struct mempool *)handle;

  if ((pool == NULL) || (free_bytes == NULL)) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if (pool->flags & ENABLE_LOCKING) {
    pthread_mutex_lock(&pool->lock);
  }

  *free_bytes = pool->mempool_item_size * pool->free_bufs_in_pool;

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }

  return 0;
}

int mempool_available_space(MemoryPoolHandle p_pool, size_t *p_avail_bytes) {
  if (!p_pool || !p_avail_bytes) {
    return S3_MEMPOOL_INVALID_ARG;
  }
  if (p_pool->flags & ENABLE_LOCKING) {
    pthread_mutex_lock(&p_pool->lock);
  }
  const size_t used_space =
      p_pool->number_of_bufs_shared * p_pool->mempool_item_size;
  const size_t max_memory_threshold = p_pool->max_memory_threshold;

  *p_avail_bytes =
      max_memory_threshold > used_space ? max_memory_threshold - used_space : 0;

  if (p_pool->flags & ENABLE_LOCKING) {
    pthread_mutex_unlock(&p_pool->lock);
  }
  return 0;
}

int mempool_getbuffer_size(MemoryPoolHandle handle, size_t *buffer_size) {
  struct mempool *pool = (struct mempool *)handle;

  if ((pool == NULL) || (buffer_size == NULL)) {
    return S3_MEMPOOL_INVALID_ARG;
  }
  *buffer_size = pool->mempool_item_size;
  return 0;
}

int mempool_downsize(MemoryPoolHandle handle, size_t mem_to_free) {
  struct mempool *pool = NULL;
  struct memory_pool_element *pool_item = NULL;
  int bufs_to_free = 0;
  int count = 0;
  char *log_msg_fmt =
      "mempool(%p): free(%p) called for pool item size(%zu) mem_to_free(%zu)";
  char log_msg[200];

  if (handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  pool = (struct mempool *)handle;

  /* pool is NULL or mem_to_free is not multiple of pool->mempool_item_size */
  if (pool == NULL || (mem_to_free % pool->mempool_item_size > 0)) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  /* Only free what we can free */
  bufs_to_free = mem_to_free / pool->mempool_item_size;
  if (bufs_to_free > pool->free_bufs_in_pool) {
    bufs_to_free = pool->free_bufs_in_pool;
  }

  /* Free the items in free list */
  if (bufs_to_free > 0) {
    pool_item = pool->free_list;
    count = 0;
    while (count < bufs_to_free && pool_item != NULL) {
      count++;
      pool->free_list = pool_item->next;
      /* Log message about free()'ed item */
      if (pool->log_callback_func) {
        snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool,
                 (void *)pool_item, pool->mempool_item_size, mem_to_free);
        pool->log_callback_func(MEMPOOL_LOG_DEBUG, log_msg);
      }
      free(pool_item);
      pool->total_bufs_allocated_by_pool--;
      pool->free_bufs_in_pool--;
      pool_item = pool->free_list;
    }
    if (pool->mem_mark_free_space_func) {
      pool->mem_mark_free_space_func(bufs_to_free * pool->mempool_item_size);
    }
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }
  return 0;
}

int mempool_destroy(MemoryPoolHandle *handle) {
  struct mempool *pool = NULL;
  struct memory_pool_element *pool_item;
  char *log_msg_fmt = "mempool(%p): free(%p) called for buffer size(%zu)";
  char log_msg[200];

  if (handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  pool = (struct mempool *)*handle;
  if (pool == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  if (*handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  /* reset the handle */
  *handle = NULL;
  /* Free the items in free list */
  pool_item = pool->free_list;
  while (pool_item != NULL) {
    pool->free_list = pool_item->next;
    /* Log message about free()'ed item */
    if (pool->log_callback_func) {
      snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool,
               (void *)pool_item, pool->mempool_item_size);
      pool->log_callback_func(MEMPOOL_LOG_DEBUG, log_msg);
    }
    free(pool_item);
#if 0
    /* Need this if below asserts are there */
    pool->total_bufs_allocated_by_pool--;
    pool->free_bufs_in_pool--;
#endif
    pool_item = pool->free_list;
  }
  pool->free_list = NULL;

  /* TODO: libevhtp/libevent seems to hold some references and not release back
   * to pool. Bug will be logged for this to investigate.
   */
  /* Assert if there are leaks */
  /*
    assert(pool->total_bufs_allocated_by_pool == 0);
    assert(pool->number_of_bufs_shared == 0);
    assert(pool->free_bufs_in_pool == 0);
  */

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
  }

  free(pool);
  pool = NULL;
  return 0;
}

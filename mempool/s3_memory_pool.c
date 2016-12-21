/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 5-Dec-2016
 */

#include "s3_memory_pool.h"

/**
 * Internal function to preallocate items to memory pool.
 * args:
 * pool (in) pointer to memory pool
 * extra_free_list (in) Extra items to be allocated
 * returns:
 * 0 on success, otherwise an error
 */
int freelist_allocate(struct mempool *pool, int extra_free_list) {
  int i;
  int rc = 0;
  void *buf = NULL;
  struct memory_pool_element *pool_item;

  if (pool == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  /* Bail out in case if we cross the max free ist size by adding
   * extra_free_list */
  if ((extra_free_list + pool->free_pool_items_count) >
      pool->free_list_max_size) {
    return S3_MEMPOOL_ERROR;
  }

  for (i = 0; i < extra_free_list; i++) {
    if (pool->flags & CREATE_ALIGNED_MEMORY) {
      rc = posix_memalign(&buf, pool->alignment, pool->mempool_item_size);
    } else {
      buf = malloc(pool->mempool_item_size);
    }
    if (buf == NULL || rc != 0) {
      return S3_MEMPOOL_ERROR;
    }
    pool->total_outstanding_memory_alloc++;
    /* Put the allocated memory into the list */
    pool_item = (struct memory_pool_element *)buf;

    pool_item->next = pool->free_list;
    pool->free_list = pool_item;
    /**memory is pre appended to list */

    /* Increase the free list count */
    pool->free_pool_items_count++;
  }
  return 0;
}

int mempool_create(size_t pool_item_size, size_t pool_initial_size,
                   size_t pool_expansion_size, size_t pool_max_theshold_size,
                   int flags, MemoryPoolHandle *handle) {
  int rc;
  int free_list_max_size;
  int allocate_free_list_size;
  struct mempool *pool = NULL;

  if (pool_item_size == 0 || pool_max_theshold_size == 0 || handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  /* Minimum size of the pool's buffer will be sizeof pointer */
  if (pool_item_size < sizeof(struct memory_pool_element)) {
    pool_item_size = sizeof(struct memory_pool_element);
  }

  *handle = NULL;

  /* Bail out if the user provides initial size less than maximum threshold size
   */
  if (pool_max_theshold_size < pool_initial_size) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  /* Calculate the number of maximum free list possible from the threshold value
   */
  free_list_max_size = pool_max_theshold_size / pool_item_size;

  pool = (struct mempool *)calloc(1, sizeof(struct mempool));
  if (pool == NULL) {
    return S3_MEMPOOL_ERROR;
  }

  /* flag that can be used to figure out whether we are doing preallocation of
   * items when creating pool */
  if (pool_initial_size != 0) {
    pool->flags |= PREALLOCATE_MEM_ON_CREATE;
  }

  pool->flags |= flags;
  pool->free_list_max_size = free_list_max_size;
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

  pool->expandable_size = pool_expansion_size;
  pool->max_memory_threshold = pool_max_theshold_size;
  /* figure out the size of free list to be preallocated from given initial pool
   * size */
  allocate_free_list_size = pool_initial_size / pool_item_size;
  *handle = (MemoryPoolHandle)pool;

  /* Allocate the free list */
  if (allocate_free_list_size > 0) {
    rc = freelist_allocate(pool, allocate_free_list_size);
    if (rc != 0) {
      goto fail;
    }
  }
  return 0;

fail:
  mempool_destroy(handle);
  return S3_MEMPOOL_ERROR;
}

void *mempool_getbuffer(MemoryPoolHandle handle, int flags) {
  int rc;
  int number_of_extra_freelist;
  struct memory_pool_element *pool_item = NULL;
  struct mempool *pool = (struct mempool *)handle;

  if (pool == NULL) {
    return NULL;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  /* If the free list is empty and pool is dynamic then expand the pool's free
   * list */
  if ((pool->expandable_size != 0) && (pool->free_pool_items_count == 0)) {
    /* Check whether we reached threshold value on memory consumption */
    if ((pool->number_of_allocation * pool->mempool_item_size) >=
        pool->max_memory_threshold) {
      if ((pool->flags & ENABLE_LOCKING) != 0) {
        pthread_mutex_unlock(&pool->lock);
      }
      /* The memory consumed by the system has crossed the threshold, so return
       * NULL */
      return NULL;
    } else if ((pool->number_of_allocation * pool->mempool_item_size +
                pool->expandable_size) > pool->max_memory_threshold) {
      /* The memory consumed by system has not crossed the threshold value,
       * however
       * if we now expand the pool by expandable_size it will cross threshold
       * so we can expand it by a value which is less than
       * max_memory_threshold
       */
      size_t extra_freelist_allocation =
          pool->max_memory_threshold -
          ((pool->number_of_allocation * pool->mempool_item_size) +
           (pool->expandable_size));
      number_of_extra_freelist =
          extra_freelist_allocation / pool->mempool_item_size;
    } else {
      number_of_extra_freelist =
          pool->expandable_size / pool->mempool_item_size;
    }

    if (number_of_extra_freelist != 0) {
      /* Expand the pool */
      rc = freelist_allocate(pool, number_of_extra_freelist);
      if (rc != 0) {
        return NULL;
      }
    }
  }

  /* Done with expansion of pool in case of pre allocated pools */

  /* Logic of allocation from free list */
  /* If there is an item on the pool's free list, then take that... */
  if (pool->free_list != NULL) {
    pool_item = pool->free_list;
    pool->free_list = pool_item->next;
    pool->free_pool_items_count--;

    if ((flags & ZEROED_ALLOCATION) != 0) {
      memset(pool_item, 0, pool->mempool_item_size);
    }
  }

  pool->number_of_allocation++;

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }

  /* allocate a new one, if item not in pool */
  if (pool_item == NULL) {
    void *buf;

    if (pool->total_outstanding_memory_alloc >= pool->free_list_max_size) {
      /* Any further memory allocation will cross the max threshold value, so
       * bail out */
      if (flags & ENABLE_LOCKING) {
        pthread_mutex_lock(&pool->lock);
        /* Not successful in providing the item */
        pool->number_of_allocation--;
        pthread_mutex_unlock(&pool->lock);
      } else {
        pool->number_of_allocation--;
      }

      return NULL;
    }

    /* allocate a new one, as item not in pool */
    if (flags & CREATE_ALIGNED_MEMORY) {
      /* Do memory aligned allocations in case of memory aligned pools */
      rc = posix_memalign(&buf, pool->alignment, pool->mempool_item_size);
      if (rc != 0) {
        return NULL;
      }
      pool_item = (struct memory_pool_element *)buf;
    } else {
      pool_item = (struct memory_pool_element *)malloc(pool->mempool_item_size);
      if (pool_item != NULL) {
        pool->total_outstanding_memory_alloc++;
      }
    }
  }
  if (pool_item == NULL) {
    if (flags & ENABLE_LOCKING) {
      pthread_mutex_lock(&pool->lock);
      /* Not successful in providing the item */
      pool->number_of_allocation--;
      pthread_mutex_unlock(&pool->lock);
    } else {
      pool->number_of_allocation--;
    }
  }

  return (void *)pool_item;
}

int mempool_releasebuffer(MemoryPoolHandle handle, void *buf) {
  struct mempool *pool = (struct mempool *)handle;
  struct memory_pool_element *pool_item = (struct memory_pool_element *)buf;

  if ((pool == NULL) || (pool_item == NULL)) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  if (pool->free_pool_items_count < pool->free_list_max_size) {
    pool_item->next = pool->free_list;
    pool->free_list = pool_item;
    pool->free_pool_items_count++;
    pool_item = NULL;
  }

  pool->number_of_allocation--;

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }

  /* Implies that we have not hooked the item to the list, so free it */
  /* This condition won't happen currently as max threshold is enforced by
   * default */
  if (pool_item != NULL) {
    /* This assert need to be removed when we dont want to enforce max threshold
     */
    assert(pool_item == NULL);
    free(pool_item);
    pool->total_outstanding_memory_alloc--;
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
  poolinfo->free_list_max_size = pool->free_list_max_size;
  poolinfo->free_pool_items_count = pool->free_pool_items_count;
  poolinfo->number_of_allocation = pool->number_of_allocation;
  poolinfo->expandable_size = pool->expandable_size;
  poolinfo->total_outstanding_memory_alloc =
      pool->total_outstanding_memory_alloc;
  poolinfo->flags = pool->flags;

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
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

int mempool_resize(MemoryPoolHandle handle, int new_max_threshold) {
  int new_max_free_list_count;
  struct memory_pool_element *pool_item;
  struct mempool *pool = (struct mempool *)handle;
  struct memory_pool_element *pool_item_tobe_freed = NULL;

  if (new_max_threshold <= 0) {
    return S3_MEMPOOL_ERROR;
  }

  /* calculate new free list size for new_max_threshold */
  new_max_free_list_count = new_max_threshold / pool->mempool_item_size;

  if (pool->flags & ENABLE_LOCKING) {
    pthread_mutex_lock(&pool->lock);
  }

  /* set the new value */
  pool->free_list_max_size = new_max_free_list_count;

  /* In case if it happens that the new max-free-list value is less than the
   * current free list then trim the free list
   */

  /* Have the list of all such excess items to be freed */
  while ((pool->free_pool_items_count > pool->free_list_max_size) &&
         (pool->free_list != NULL)) {
    pool_item = pool->free_list;
    pool->free_list = pool_item->next;
    pool_item->next = pool_item_tobe_freed;
    pool_item_tobe_freed = pool_item;
    pool->free_pool_items_count--;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }

  /* Now free the unwanted extra items */
  while (pool_item_tobe_freed != NULL) {
    pool_item = pool_item_tobe_freed;
    pool_item_tobe_freed = pool_item_tobe_freed->next;
    free(pool_item);
  }
  return 0;
}

int mempool_destroy(MemoryPoolHandle *handle) {
  struct mempool *pool;
  struct memory_pool_element *pool_item;
  struct memory_pool_element *next_item;

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

  /* Free the items in free list */
  for (pool_item = pool->free_list; pool_item != NULL;) {
    next_item = pool_item->next;
    free(pool_item);
    pool->total_outstanding_memory_alloc--;
    pool_item = next_item;
  }

  /* Assert if total memory allocation via pool and free via pool doesn't match
   */
  assert((pool->total_outstanding_memory_alloc - pool->number_of_allocation) ==
         0);
  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
  }
  free(pool);
  /* reset the handle */
  *handle = NULL;
  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_destroy(&pool->lock);
  }
  return 0;
}

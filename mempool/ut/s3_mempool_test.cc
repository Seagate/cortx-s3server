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
 * Original creation date: 14-Dec-2016
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "s3_memory_pool.c"

#define ONE_4K_BLOCK 4096
#define TWO_4K_BLOCK (2 * ONE_4K_BLOCK)
#define THREE_4K_BLOCK (3 * ONE_4K_BLOCK)
#define FOUR_4K_BLOCK (4 * ONE_4K_BLOCK)
#define SIX_4K_BLOCK (6 * ONE_4K_BLOCK)

class MempoolSelfCreateTestSuite : public testing::Test {
 protected:
  void SetUp() {
    first_handle = NULL;
    second_handle = NULL;
  }

  void Teardown() {
    if (first_handle != NULL) {
      mempool_destroy(&first_handle);
    }
    if (second_handle != NULL) {
      mempool_destroy(&second_handle);
    }
  }

  MemoryPoolHandle first_handle;
  MemoryPoolHandle second_handle;
  struct pool_info firstpass_pool_details;
  struct pool_info secondpass_pool_details;
};

class MempoolTestSuite : public testing::Test {
 protected:
  void SetUp() {
    mempool_create(ONE_4K_BLOCK, THREE_4K_BLOCK, TWO_4K_BLOCK, SIX_4K_BLOCK,
                   CREATE_ALIGNED_MEMORY | ENABLE_LOCKING | ZEROED_ALLOCATION,
                   &handle);
  }

  void Teardown() { mempool_destroy(&handle); }

  MemoryPoolHandle handle;
  struct pool_info firstpass_pool_details;
  struct pool_info secondpass_pool_details;
};

TEST_F(MempoolSelfCreateTestSuite, CreatePoolTest) {
  EXPECT_EQ(
      0, mempool_create(ONE_4K_BLOCK, 0, 0, THREE_4K_BLOCK, 0, &first_handle));
  EXPECT_NE(first_handle, (void *)NULL);
  EXPECT_EQ(0, mempool_getinfo(first_handle, &firstpass_pool_details));
  EXPECT_EQ(3, firstpass_pool_details.free_list_max_size);
  EXPECT_EQ(0, firstpass_pool_details.free_pool_items_count);
  EXPECT_EQ(0, firstpass_pool_details.number_of_allocation);
  EXPECT_EQ(0, firstpass_pool_details.total_outstanding_memory_alloc);
  EXPECT_EQ(ONE_4K_BLOCK, firstpass_pool_details.mempool_item_size);
  EXPECT_EQ(0, firstpass_pool_details.expandable_size);
  memset(&firstpass_pool_details, 0, sizeof(firstpass_pool_details));

  EXPECT_EQ(0, mempool_create(
                   ONE_4K_BLOCK, THREE_4K_BLOCK, TWO_4K_BLOCK, SIX_4K_BLOCK,
                   CREATE_ALIGNED_MEMORY | ENABLE_LOCKING | ZEROED_ALLOCATION,
                   &second_handle));
  EXPECT_NE(second_handle, (void *)NULL);
  EXPECT_EQ(0, mempool_getinfo(second_handle, &firstpass_pool_details));
  EXPECT_EQ(6, firstpass_pool_details.free_list_max_size);
  EXPECT_EQ(3, firstpass_pool_details.free_pool_items_count);
  EXPECT_EQ(0, firstpass_pool_details.number_of_allocation);
  EXPECT_EQ(3, firstpass_pool_details.total_outstanding_memory_alloc);
  EXPECT_EQ(ONE_4K_BLOCK, firstpass_pool_details.mempool_item_size);
  EXPECT_EQ(TWO_4K_BLOCK, firstpass_pool_details.expandable_size);
  EXPECT_TRUE(CREATE_ALIGNED_MEMORY & firstpass_pool_details.flags);
  EXPECT_TRUE(ENABLE_LOCKING & firstpass_pool_details.flags);
  EXPECT_TRUE(ZEROED_ALLOCATION & firstpass_pool_details.flags);
  mempool_destroy(&first_handle);
  mempool_destroy(&second_handle);
}

TEST_F(MempoolSelfCreateTestSuite, CreatePoolNegativeTest) {
  EXPECT_NE(0, mempool_create(ONE_4K_BLOCK, 0, 0, 0, 0, &first_handle));
  EXPECT_EQ(first_handle, (void *)NULL);
  EXPECT_NE(0, mempool_create(0, 0, 0, 0, 0, &second_handle));
  EXPECT_EQ(second_handle, (void *)NULL);
  EXPECT_NE(0, mempool_create(ONE_4K_BLOCK, 0, 0, THREE_4K_BLOCK, 0, NULL));
}

TEST_F(MempoolSelfCreateTestSuite, DestroyPoolTest) {
  EXPECT_EQ(
      0, mempool_create(ONE_4K_BLOCK, 0, 0, THREE_4K_BLOCK, 0, &first_handle));
  EXPECT_EQ(0, mempool_destroy(&first_handle));
  EXPECT_EQ(first_handle, (void *)NULL);
}

TEST_F(MempoolTestSuite, AllocateMemMemoryPoolTest) {
  unsigned int i;
  char *buf = NULL;
  buf = (char *)mempool_getbuffer(handle, ZEROED_ALLOCATION);
  EXPECT_NE(buf, (void *)NULL);

  EXPECT_EQ(0, mempool_getinfo(handle, &firstpass_pool_details));
  buf = (char *)mempool_getbuffer(handle, ZEROED_ALLOCATION);
  EXPECT_NE(buf, (void *)NULL);
  EXPECT_EQ(0, mempool_getinfo(handle, &secondpass_pool_details));
  EXPECT_EQ(firstpass_pool_details.free_list_max_size,
            secondpass_pool_details.free_list_max_size);
  // After memory allocation from pool, pool's free_pool_items_count should
  // decrease
  EXPECT_EQ(secondpass_pool_details.free_pool_items_count,
            (firstpass_pool_details.free_pool_items_count - 1));
  // Number of buffer allocated to user should increase
  EXPECT_EQ(secondpass_pool_details.number_of_allocation,
            (firstpass_pool_details.number_of_allocation + 1));

  EXPECT_EQ(secondpass_pool_details.total_outstanding_memory_alloc,
            firstpass_pool_details.total_outstanding_memory_alloc);

  // The buffer should be zeroed out as flag ZEROED_ALLOCATION was passed
  for (i = 0; i < firstpass_pool_details.mempool_item_size; i++) {
    EXPECT_EQ(0, buf[i]);
  }

  // Ensure buffer is 4k aligned, when CREATE_ALIGNED_MEMORY flag is passed
  // during pool creation
  EXPECT_TRUE(((uint64_t)buf & 4095) == 0);
}

// Test to check whether native memory allocation methods
// (malloc/posix_memalign)
// gets called when pool's free list is empty
TEST_F(MempoolSelfCreateTestSuite, MemAllocateNativeTest) {
  void *buf;
  EXPECT_EQ(
      0, mempool_create(ONE_4K_BLOCK, 0, 0, THREE_4K_BLOCK, 0, &first_handle));
  EXPECT_NE(first_handle, (void *)NULL);
  EXPECT_EQ(0, mempool_getinfo(first_handle, &firstpass_pool_details));
  // No buffer allocated
  EXPECT_EQ(0, firstpass_pool_details.total_outstanding_memory_alloc);
  // free list is empty
  EXPECT_EQ(0, firstpass_pool_details.free_pool_items_count);

  buf = mempool_getbuffer(first_handle, 0);
  EXPECT_NE(buf, (void *)NULL);

  EXPECT_EQ(0, mempool_getinfo(first_handle, &secondpass_pool_details));
  // One buffer allocated via posix_memalign/malloc
  EXPECT_EQ(1, secondpass_pool_details.total_outstanding_memory_alloc);
  EXPECT_EQ(0, secondpass_pool_details.free_pool_items_count);
  mempool_destroy(&first_handle);
}

// Test to check whether maximum threshold in pool is reached any time
TEST_F(MempoolSelfCreateTestSuite, MaxThresholdTest) {
  void *first_buf;
  void *second_buf;
  void *third_buf;
  void *fourth_buf;
  EXPECT_EQ(
      0, mempool_create(ONE_4K_BLOCK, 0, 0, THREE_4K_BLOCK, 0, &first_handle));
  first_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_NE(first_buf, (void *)NULL);
  second_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_NE(second_buf, (void *)NULL);
  third_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_NE(third_buf, (void *)NULL);

  // This time allocation will cross the threshold value
  fourth_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_EQ(fourth_buf, (void *)NULL);

  mempool_releasebuffer(first_handle, first_buf);
  mempool_releasebuffer(first_handle, second_buf);
  mempool_releasebuffer(first_handle, third_buf);
  mempool_destroy(&first_handle);
}

// Test to check the pool expansion
TEST_F(MempoolSelfCreateTestSuite, PoolExpansionTest) {
  void *first_buf;
  void *second_buf;
  void *third_buf;
  void *fourth_buf;
  void *fifth_buf;
  EXPECT_EQ(0, mempool_create(ONE_4K_BLOCK, ONE_4K_BLOCK, THREE_4K_BLOCK,
                              FOUR_4K_BLOCK, 0, &first_handle));

  first_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_NE(first_buf, (void *)NULL);

  // Now there is no buffer in pool's free list
  // mempool_alloc should increase the pool's free list by three buffers
  second_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_NE(second_buf, (void *)NULL);
  EXPECT_EQ(0, mempool_getinfo(first_handle, &firstpass_pool_details));
  // Increase in free buffer count by 2 (pool expanded by 3 buffers and then one
  // given to user)
  EXPECT_EQ(2, firstpass_pool_details.free_pool_items_count);
  // Consume two buffers
  third_buf = mempool_getbuffer(first_handle, 0);
  fourth_buf = mempool_getbuffer(first_handle, 0);

  fifth_buf = mempool_getbuffer(first_handle, 0);
  EXPECT_EQ(fifth_buf, (void *)NULL);
  mempool_destroy(&first_handle);
  free(first_buf);
  free(second_buf);
  free(third_buf);
  free(fourth_buf);
}

TEST_F(MempoolTestSuite, MemPoolFreeTest) {
  void *buf;
  EXPECT_EQ(0, mempool_getinfo(handle, &firstpass_pool_details));
  buf = mempool_getbuffer(handle, 0);
  EXPECT_NE(buf, (void *)NULL);

  mempool_releasebuffer(handle, buf);
  EXPECT_EQ(0, mempool_getinfo(handle, &secondpass_pool_details));
  EXPECT_EQ(firstpass_pool_details.free_pool_items_count,
            secondpass_pool_details.free_pool_items_count);
}

TEST_F(MempoolTestSuite, MemPoolResizeTest) {
  EXPECT_EQ(0, mempool_getinfo(handle, &firstpass_pool_details));
  EXPECT_EQ(0, mempool_resize(handle, TWO_4K_BLOCK));
  EXPECT_EQ(0, mempool_getinfo(handle, &secondpass_pool_details));
  EXPECT_EQ(2, secondpass_pool_details.free_list_max_size);
  EXPECT_EQ(2, secondpass_pool_details.free_pool_items_count);

  memset(&firstpass_pool_details, 0, sizeof(firstpass_pool_details));
  EXPECT_EQ(0, mempool_resize(handle, SIX_4K_BLOCK));
  EXPECT_EQ(0, mempool_getinfo(handle, &firstpass_pool_details));

  EXPECT_EQ(6, firstpass_pool_details.free_list_max_size);
}

int main(int argc, char **argv) {
  int rc;

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  rc = RUN_ALL_TESTS();

  return rc;
}

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

#include "s3_memory_pool.h"

#define FOUR_KB 4096
#define EIGHT_KB (2 * FOUR_KB)
#define TWELVE_KB (3 * FOUR_KB)
#define SIXTEEN_KB (4 * FOUR_KB)
#define TWENTYFOUR_KB (6 * FOUR_KB)

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
    handle = NULL;
    //             BUF_SZ , initial  , expandable_size, max
    mempool_create(FOUR_KB, TWELVE_KB, EIGHT_KB, TWENTYFOUR_KB,
                   (func_log_callback_type)NULL,
                   CREATE_ALIGNED_MEMORY | ENABLE_LOCKING | ZEROED_BUFFER,
                   &handle);
  }

  void Teardown() { mempool_destroy(&handle); }

  MemoryPoolHandle handle;
  struct pool_info firstpass_pool_details;
  struct pool_info secondpass_pool_details;
};

TEST_F(MempoolSelfCreateTestSuite, CreatePoolTest) {
  EXPECT_EQ(
      0, mempool_create(FOUR_KB, TWELVE_KB, EIGHT_KB, TWENTYFOUR_KB,
                        (func_log_callback_type)NULL,
                        CREATE_ALIGNED_MEMORY | ENABLE_LOCKING | ZEROED_BUFFER,
                        &first_handle));
  EXPECT_NE(first_handle, (void *)NULL);
  EXPECT_EQ(0, mempool_getinfo(first_handle, &firstpass_pool_details));
  EXPECT_EQ(3, firstpass_pool_details.free_bufs_in_pool);
  EXPECT_EQ(0, firstpass_pool_details.number_of_bufs_shared);
  EXPECT_EQ(3, firstpass_pool_details.total_bufs_allocated_by_pool);
  EXPECT_EQ(FOUR_KB, firstpass_pool_details.mempool_item_size);
  EXPECT_EQ(EIGHT_KB, firstpass_pool_details.expandable_size);
  EXPECT_TRUE(CREATE_ALIGNED_MEMORY & firstpass_pool_details.flags);
  EXPECT_TRUE(ENABLE_LOCKING & firstpass_pool_details.flags);
  EXPECT_TRUE(ZEROED_BUFFER & firstpass_pool_details.flags);
  mempool_destroy(&first_handle);
}

TEST_F(MempoolSelfCreateTestSuite, CreatePoolNegativeTest) {
  EXPECT_NE(0, mempool_create(FOUR_KB, 0, 0, 0, (func_log_callback_type)NULL, 0,
                              &first_handle));
  EXPECT_EQ(first_handle, (void *)NULL);
  EXPECT_NE(0, mempool_create(0, 0, 0, 0, (func_log_callback_type)NULL, 0,
                              &second_handle));
  EXPECT_EQ(second_handle, (void *)NULL);
  EXPECT_NE(0, mempool_create(FOUR_KB, 0, 0, TWELVE_KB,
                              (func_log_callback_type)NULL, 0, NULL));
}

TEST_F(MempoolSelfCreateTestSuite, DestroyPoolTest) {
  EXPECT_EQ(0, mempool_create(FOUR_KB, TWELVE_KB, EIGHT_KB, TWENTYFOUR_KB,
                              (func_log_callback_type)NULL, 0, &first_handle));
  EXPECT_NE(first_handle, (void *)NULL);
  EXPECT_EQ(0, mempool_destroy(&first_handle));
  EXPECT_EQ(first_handle, (void *)NULL);
}



// Test to check whether native memory allocation methods
// (malloc/posix_memalign)
// gets called when pool's free list is empty
TEST_F(MempoolSelfCreateTestSuite, MemAllocateNativeTest) {
  EXPECT_EQ(0, mempool_create(FOUR_KB, 0, EIGHT_KB, TWELVE_KB,
                              (func_log_callback_type)NULL, 0, &first_handle));
  EXPECT_TRUE(first_handle != NULL);

  EXPECT_EQ(0, mempool_getinfo(first_handle, &firstpass_pool_details));
  // No buffer allocated
  EXPECT_EQ(0, firstpass_pool_details.total_bufs_allocated_by_pool);
  // free list is empty
  EXPECT_EQ(0, firstpass_pool_details.free_bufs_in_pool);

  void *buf =
      mempool_getbuffer(first_handle, firstpass_pool_details.mempool_item_size);
  EXPECT_TRUE(buf != NULL);

  EXPECT_EQ(0, mempool_getinfo(first_handle, &secondpass_pool_details));
  // Two buffer allocated via posix_memalign/malloc
  EXPECT_EQ(2, secondpass_pool_details.total_bufs_allocated_by_pool);
  EXPECT_EQ(1, secondpass_pool_details.free_bufs_in_pool);
  EXPECT_EQ(1, secondpass_pool_details.number_of_bufs_shared);

  mempool_releasebuffer(first_handle, buf,
                        firstpass_pool_details.mempool_item_size);
  mempool_destroy(&first_handle);
}

// Test to check whether maximum threshold in pool is reached any time
TEST_F(MempoolSelfCreateTestSuite, MaxThresholdTest) {
  EXPECT_EQ(0, mempool_create(FOUR_KB, 0, FOUR_KB, TWELVE_KB,
                              (func_log_callback_type)NULL, 0, &first_handle));
  size_t avail_space;
  mempool_available_space(first_handle, &avail_space);
  EXPECT_EQ(avail_space, TWELVE_KB);

  void *first_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(first_buf != NULL);

  void *second_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(second_buf != NULL);

  void *third_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(third_buf != NULL);

  mempool_available_space(first_handle, &avail_space);
  EXPECT_EQ(avail_space, 0);

  // This time allocation will cross the threshold value
  void *fourth_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(fourth_buf == NULL);

  mempool_releasebuffer(first_handle, first_buf, FOUR_KB);
  mempool_releasebuffer(first_handle, second_buf, FOUR_KB);
  mempool_releasebuffer(first_handle, third_buf, FOUR_KB);
  mempool_destroy(&first_handle);
}

// Test to check mempool free space
TEST_F(MempoolSelfCreateTestSuite, MempoolFreeSpace) {
  size_t free_bytes = 0;
  size_t avail_space = 0;
  EXPECT_EQ(0, mempool_create(FOUR_KB, EIGHT_KB, FOUR_KB, TWELVE_KB,
                              (func_log_callback_type)NULL, 0, &first_handle));

  EXPECT_EQ(0, mempool_reserved_space(first_handle, &free_bytes));
  EXPECT_EQ(EIGHT_KB, free_bytes);

  EXPECT_EQ(0, mempool_available_space(first_handle, &avail_space));
  EXPECT_EQ(TWELVE_KB, avail_space);

  mempool_destroy(&first_handle);
}

// Test to check mempool free space
TEST_F(MempoolSelfCreateTestSuite, MempoolFreeSpaceInvalid) {
  EXPECT_EQ(0, mempool_create(FOUR_KB, FOUR_KB, FOUR_KB, TWELVE_KB,
                              (func_log_callback_type)NULL, 0, &first_handle));
  EXPECT_EQ(S3_MEMPOOL_INVALID_ARG, mempool_reserved_space(first_handle, NULL));
  EXPECT_EQ(S3_MEMPOOL_INVALID_ARG,
            mempool_available_space(first_handle, NULL));
  size_t avail_space;
  EXPECT_EQ(S3_MEMPOOL_INVALID_ARG,
            mempool_available_space(NULL, &avail_space));
  mempool_destroy(&first_handle);
}

// Test to check the pool expansion
TEST_F(MempoolSelfCreateTestSuite, PoolExpansionTest) {
  EXPECT_EQ(0, mempool_create(FOUR_KB, FOUR_KB, TWELVE_KB, SIXTEEN_KB,
                              (func_log_callback_type)NULL, 0, &first_handle));

  void *first_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(first_buf != NULL);

  // Now there is no buffer in pool's free list
  // mempool_alloc should increase the pool's free list by three buffers
  void *second_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(second_buf != NULL);

  EXPECT_EQ(0, mempool_getinfo(first_handle, &firstpass_pool_details));
  // Increase in free buffer count by 2 (pool expanded by 3 buffers and then one
  // given to user)
  EXPECT_EQ(2, firstpass_pool_details.free_bufs_in_pool);

  // Consume two buffers
  void *third_buf = mempool_getbuffer(first_handle, FOUR_KB);
  void *fourth_buf = mempool_getbuffer(first_handle, FOUR_KB);

  // Max threshold
  void *fifth_buf = mempool_getbuffer(first_handle, FOUR_KB);
  EXPECT_TRUE(fifth_buf == NULL);

  // Release buffers back to pool
  mempool_releasebuffer(first_handle, first_buf, FOUR_KB);
  mempool_releasebuffer(first_handle, second_buf, FOUR_KB);
  mempool_releasebuffer(first_handle, third_buf, FOUR_KB);
  mempool_releasebuffer(first_handle, fourth_buf, FOUR_KB);

  mempool_destroy(&first_handle);
}

TEST_F(MempoolTestSuite, MemPoolFreeTest) {
  EXPECT_EQ(0, mempool_getinfo(handle, &firstpass_pool_details));

  void *buf =
      mempool_getbuffer(handle, firstpass_pool_details.mempool_item_size);
  EXPECT_TRUE(buf != NULL);

  mempool_releasebuffer(handle, buf, firstpass_pool_details.mempool_item_size);

  EXPECT_EQ(0, mempool_getinfo(handle, &secondpass_pool_details));
  EXPECT_EQ(firstpass_pool_details.free_bufs_in_pool,
            secondpass_pool_details.free_bufs_in_pool);
}

TEST_F(MempoolTestSuite, MemPoolBufferSizeTest) {
  size_t buffer_size = 0;
  EXPECT_EQ(0, mempool_getbuffer_size(handle, &buffer_size));
  EXPECT_EQ(FOUR_KB, buffer_size);
}

TEST_F(MempoolTestSuite, MemPoolNegativeBufferSizeTest) {
  size_t buffer_size = 0;
  EXPECT_EQ(S3_MEMPOOL_INVALID_ARG, mempool_getbuffer_size(NULL, &buffer_size));
  EXPECT_EQ(S3_MEMPOOL_INVALID_ARG, mempool_getbuffer_size(handle, NULL));
}

TEST_F(MempoolTestSuite, AllocateMemMemoryPoolTest) {
  void *buf =
      mempool_getbuffer(handle, firstpass_pool_details.mempool_item_size);
  EXPECT_TRUE(buf != NULL);

  EXPECT_EQ(0, mempool_getinfo(handle, &firstpass_pool_details));
  EXPECT_EQ(FOUR_KB, firstpass_pool_details.mempool_item_size);

  buf = mempool_getbuffer(handle, firstpass_pool_details.mempool_item_size);
  EXPECT_TRUE(buf != NULL);

  EXPECT_EQ(0, mempool_getinfo(handle, &secondpass_pool_details));

  // After memory allocation from pool, pool's free_bufs_in_pool should
  // decrease
  EXPECT_EQ(secondpass_pool_details.free_bufs_in_pool,
            (firstpass_pool_details.free_bufs_in_pool - 1));
  // Number of buffer allocated to user should increase
  EXPECT_EQ(secondpass_pool_details.number_of_bufs_shared,
            (firstpass_pool_details.number_of_bufs_shared + 1));

  EXPECT_EQ(secondpass_pool_details.total_bufs_allocated_by_pool,
            firstpass_pool_details.total_bufs_allocated_by_pool);

  // The buffer should be zeroed out as flag ZEROED_BUFFER was passed
  char *ptr = (char *)buf;
  for (unsigned int i = 0; i < firstpass_pool_details.mempool_item_size; i++) {
    EXPECT_EQ(0, ptr[i]);
  }

  // Ensure buffer is 4k aligned, when CREATE_ALIGNED_MEMORY flag is passed
  // during pool creation
  EXPECT_TRUE(((uint64_t)buf & 4095) == 0);
}

int main(int argc, char **argv) {
  int rc;

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  rc = RUN_ALL_TESTS();

  return rc;
}

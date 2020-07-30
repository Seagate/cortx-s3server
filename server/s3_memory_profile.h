#pragma once

#ifndef __S3_SERVER_S3_MEMORY_PROFILE_H__
#define __S3_SERVER_S3_MEMORY_PROFILE_H__

class S3MemoryProfile {
  size_t memory_per_put_request(int layout_id);

 public:
  // Returns true if we have enough memory in mempool to process
  // either put request. Get request we just reject when we run
  // out of memory, memory pool manager is dynamic and free space
  // blocked by less used unit_size, so we cannot get accurate estimate
  virtual bool we_have_enough_memory_for_put_obj(int layout_id);
  virtual bool free_memory_in_pool_above_threshold_limits();
};

#endif

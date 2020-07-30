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

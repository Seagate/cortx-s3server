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

#pragma once

#ifndef __S3_SERVER_S3_MEMORY_PROFILE_H__
#define __S3_SERVER_S3_MEMORY_PROFILE_H__

#include <cstddef>
#include <cstdint>

class S3MemoryProfile {
  size_t libevent_pool_max_threshold;
  size_t libevent_pool_reserve_size;
  size_t read_ahead_pool_reserve;
  uint_fast8_t read_ahead_pool_usage_ratio;

  size_t memory_per_put_request(int layout_id);

  enum {
    DEFAULT_PUT_OBJECT_READ_AHEAD_GROWTH_RATIO = 50
  };

 protected:
  // For GMock
  S3MemoryProfile(size_t libevent_pool_max_threshold_,
                  size_t libevent_pool_reserve_size_ = 0,
                  size_t read_ahead_pool_reserve_ = 0,
                  uint_fast8_t read_ahead_pool_usage_ratio_ =
                      DEFAULT_PUT_OBJECT_READ_AHEAD_GROWTH_RATIO);

 public:
  S3MemoryProfile();
  virtual ~S3MemoryProfile();  // A derived class is used in UTs

  // Returns true if we have enough memory in mempool to process
  // either put request. Get request we just reject when we run
  // out of memory, memory pool manager is dynamic and free space
  // blocked by less used unit_size, so we cannot get accurate estimate
  virtual bool we_have_enough_memory_for_put_obj(int layout_id);

  virtual bool free_memory_in_pool_above_threshold_limits();

  virtual size_t max_read_ahead_size() const;

  virtual size_t free_space_in_pool() const;
};

#endif

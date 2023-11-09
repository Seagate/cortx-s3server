/*
 * Copyright (c) 2020-2021 Seagate Technology LLC and/or its Affiliates
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

#ifndef __S3_SERVER_STATS_H__
#define __S3_SERVER_STATS_H__

#include <cstdint>
#include <string>

#include "s3_option.h"
#include "socket_wrapper.h"

class S3Stats {
 public:
  // Increase/decrease count for `key` by `value`
  virtual int count(const std::string& key, std::int64_t value, int retry = 1,
                    float sample_rate = 1.0) = 0;

  // Log the time in ms for `key`
  virtual int timing(const std::string& key, std::size_t time_ms, int retry = 1,
                     float sample_rate = 1.0) = 0;

  // Set the initial gauge value for `key`
  virtual int set_gauge(const std::string& key, int value, int retry = 1) = 0;

  // Increase/decrease gauge for `key` by `value`
  virtual int update_gauge(const std::string& key, int value,
                           int retry = 1) = 0;

  // Count unique `value`s for a `key`
  virtual int count_unique(const std::string& key, const std::string& value,
                           int retry = 1) = 0;

  virtual int init() = 0;

  virtual void finish() = 0;

  virtual ~S3Stats() = default;
};

class S3StatsManager {
 public:
  static S3Stats* get_instance(SocketInterface* socket_obj = NULL);
  static void delete_instance();

 private:
  static S3Stats* stats_instance;
};

extern S3Option* g_option_instance;
extern S3Stats* g_stats_instance;

// Utility Wrappers for StatsD
int s3_stats_init();
void s3_stats_fini();

static inline int s3_stats_inc(const std::string& key, int retry = 1,
                               float sample_rate = 1.0) {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  return g_stats_instance->count(key, 1, retry, sample_rate);
}

static inline int s3_stats_dec(const std::string& key, int retry = 1,
                               float sample_rate = 1.0) {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  return g_stats_instance->count(key, -1, retry, sample_rate);
}

static inline int s3_stats_count(const std::string& key, std::int64_t value,
                                 int retry = 1, float sample_rate = 1.0) {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  return g_stats_instance->count(key, value, retry, sample_rate);
}

int s3_stats_timing(const std::string& key, std::size_t value, int retry = 1,
                    float sample_rate = 1.0);

static inline int s3_stats_set_gauge(const std::string& key, int value,
                                     int retry = 1) {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  return g_stats_instance->set_gauge(key, value, retry);
}

static inline int s3_stats_update_gauge(const std::string& key, int value,
                                        int retry = 1) {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  return g_stats_instance->update_gauge(key, value, retry);
}

static inline int s3_stats_count_unique(const std::string& key,
                                        const std::string& value,
                                        int retry = 1) {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  return g_stats_instance->count_unique(key, value, retry);
}

#endif  // __S3_SERVER_STATS_H__

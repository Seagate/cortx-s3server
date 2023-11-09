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

#include "s3_stats.h"
#include "s3_statsd_stats.h"

#include <string>

S3Stats* g_stats_instance = NULL;
S3Stats* S3StatsManager::stats_instance = NULL;

S3Stats* S3StatsManager::get_instance(SocketInterface* socket_obj) {
  if (!stats_instance) {
    switch (g_option_instance->get_stats_aggregator_backend_type()) {
      case StatsAggregatorBackendType::STATSD:
        stats_instance =
            new S3StatsdStats(g_option_instance->get_statsd_ip_addr(),
                              g_option_instance->get_statsd_port(), socket_obj);
        break;
      default:
        break;
    }
    if (stats_instance && stats_instance->init() != 0) {
      delete stats_instance;
      stats_instance = NULL;
    }
  }
  return stats_instance;
}

void S3StatsManager::delete_instance() {
  if (stats_instance) {
    stats_instance->finish();
    delete stats_instance;
    stats_instance = NULL;
  }
}

int s3_stats_init() {
  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  if (!g_stats_instance) {
    g_stats_instance = S3StatsManager::get_instance();
  }
  if (g_stats_instance) {
    return 0;
  } else {
    return -1;
  }
}

void s3_stats_fini() {
  if (!g_option_instance->is_stats_enabled()) {
    return;
  }
  if (g_stats_instance) {
    S3StatsManager::delete_instance();
    g_stats_instance = NULL;
  }
}

int s3_stats_timing(const std::string& key, size_t value, int retry,
                    float sample_rate) {

  if (!g_option_instance->is_stats_enabled()) {
    return 0;
  }
  if (value == (size_t)(-1)) {
    s3_log(S3_LOG_ERROR, "", "Invalid time value for key %s\n", key.c_str());
    errno = EINVAL;
    return -1;
  }
  return g_stats_instance->timing(key, value, retry, sample_rate);
}

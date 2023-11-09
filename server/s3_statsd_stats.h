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

#ifndef __S3_SERVER_STATSD_STATS_H__
#define __S3_SERVER_STATSD_STATS_H__

#include <cstdint>
#include <gtest/gtest_prod.h>
#include <limits>
#include <math.h>
#include <string>
#include <unordered_set>

#include "s3_log.h"
#include "s3_stats.h"
#include "socket_wrapper.h"

class S3StatsdStats : public S3Stats {
 public:
  virtual int count(const std::string& key, std::int64_t value, int retry = 1,
                    float sample_rate = 1.0) override;

  virtual int timing(const std::string& key, std::size_t time_ms, int retry = 1,
                     float sample_rate = 1.0) override;

  virtual int set_gauge(const std::string& key, int value,
                        int retry = 1) override;

  virtual int update_gauge(const std::string& key, int value,
                           int retry = 1) override;

  virtual int count_unique(const std::string& key, const std::string& value,
                           int retry = 1) override;

  virtual int init() override;

  virtual void finish() override;

  S3StatsdStats(const std::string& host_addr, const unsigned short port_num,
                SocketInterface* socket_obj_ptr = NULL)
      : host(host_addr), port(port_num), sock(-1) {
    s3_log(S3_LOG_DEBUG, "", "%s Ctor\n", __func__);
    metrics_allowlist.clear();
    if (socket_obj_ptr) {
      socket_obj.reset(socket_obj_ptr);
    } else {
      socket_obj.reset(new SocketWrapper());
    }
  }

 private:
  // Load & parse the allowlist file
  int load_allowlist();

  // Check if metric is present in allowlist
  bool is_allowed_to_publish(const std::string& key) {
    return metrics_allowlist.find(key) != metrics_allowlist.end();
  }

  // Send message to server
  int send(const std::string& msg, int retry = 1);

  // Form a message in StatsD format and send it to server
  int form_and_send_msg(const std::string& key, const std::string& type,
                        const std::string& value, int retry, float sample_rate);

  // Returns true if two float numbers are (alomst) equal
  bool is_fequal(float x, float y) {
    return (fabsf(x - y) < std::numeric_limits<float>::epsilon());
  }

  // Returns true if key name is valid (doesn't have chars: `@`, `|`, or `:`)
  bool is_keyname_valid(const std::string& key) {
    std::size_t found = key.find_first_of("@|:");
    return (found == std::string::npos);
  }

  // StatsD host address & port number
  std::string host;
  unsigned short port;

  int sock;
  struct sockaddr_in server;
  std::unique_ptr<SocketInterface> socket_obj;

  // metrics allowlist
  std::unordered_set<std::string> metrics_allowlist;

  FRIEND_TEST(S3StatsTest, Init);
  FRIEND_TEST(S3StatsTest, Allowlist);
  FRIEND_TEST(S3StatsTest, S3StatsSendMustSucceedIfSocketSendToSucceeds);
  FRIEND_TEST(S3StatsTest, S3StatsSendMustRetryAndFailIfRetriesFail);
};

#endif  // __S3_SERVER_STATSD_STATS_H__

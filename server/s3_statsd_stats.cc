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

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>

#include "s3_statsd_stats.h"

int S3StatsdStats::count(const std::string& key, int64_t value, int retry,
                         float sample_rate) {
  return form_and_send_msg(key, "c", std::to_string(value), retry, sample_rate);
}

int S3StatsdStats::timing(const std::string& key, size_t time_ms, int retry,
                          float sample_rate) {
  return form_and_send_msg(key, "ms", std::to_string(time_ms), retry,
                           sample_rate);
}

int S3StatsdStats::set_gauge(const std::string& key, int value, int retry) {
  if (value < 0) {
    s3_log(S3_LOG_ERROR, "",
           "Invalid gauge value: Initial gauge value can not be negative. Key "
           "[%s], Value[%d]\n",
           key.c_str(), value);
    errno = EINVAL;
    return -1;
  } else {
    return form_and_send_msg(key, "g", std::to_string(value), retry, 1.0);
  }
}

int S3StatsdStats::update_gauge(const std::string& key, int value, int retry) {
  std::string value_str;
  if (value >= 0) {
    value_str = "+" + std::to_string(value);
  } else {
    value_str = std::to_string(value);
  }
  return form_and_send_msg(key, "g", value_str, retry, 1.0);
}

int S3StatsdStats::count_unique(const std::string& key,
                                const std::string& value, int retry) {
  return form_and_send_msg(key, "s", value, retry, 1.0);
}

int S3StatsdStats::load_allowlist() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  std::string allowlist_filename =
      g_option_instance->get_stats_allowlist_filename();
  s3_log(S3_LOG_DEBUG, "", "Loading allowlist file: %s\n",
         allowlist_filename.c_str());
  std::ifstream fstream(allowlist_filename.c_str());
  if (!fstream.good()) {
    s3_log(S3_LOG_ERROR, "", "Stats allowlist file does not exist: %s\n",
           allowlist_filename.c_str());
    return -1;
  }
  try {
    YAML::Node root_node = YAML::LoadFile(allowlist_filename);
    if (root_node.IsNull()) {
      s3_log(S3_LOG_DEBUG, "", "Stats allowlist file is empty: %s\n",
             allowlist_filename.c_str());
      return 0;
    }
    for (auto it = root_node.begin(); it != root_node.end(); ++it) {
      metrics_allowlist.insert(it->as<std::string>());
    }
  }
  catch (const YAML::RepresentationException& e) {
    s3_log(S3_LOG_ERROR, "", "YAML::RepresentationException caught: %s\n",
           e.what());
    s3_log(S3_LOG_ERROR, "", "Yaml file [%s] is incorrect\n",
           allowlist_filename.c_str());
    return -1;
  }
  catch (const YAML::ParserException& e) {
    s3_log(S3_LOG_ERROR, "", "YAML::ParserException caught: %s\n", e.what());
    s3_log(S3_LOG_ERROR, "", "Parsing Error in yaml file %s\n",
           allowlist_filename.c_str());
    return -1;
  }
  catch (const YAML::EmitterException& e) {
    s3_log(S3_LOG_ERROR, "", "YAML::EmitterException caught: %s\n", e.what());
    return -1;
  }
  catch (YAML::Exception& e) {
    s3_log(S3_LOG_ERROR, "", "YAML::Exception caught: %s\n", e.what());
    return -1;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return 0;
}

int S3StatsdStats::init() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  if (load_allowlist() == -1) {
    s3_log(S3_LOG_ERROR, "", "load_allowlist failed parsing the file: %s\n",
           g_option_instance->get_stats_allowlist_filename().c_str());
    return -1;
  }
  sock = socket_obj->socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1) {
    s3_log(S3_LOG_ERROR, "", "socket call failed: %s\n", strerror(errno));
    return -1;
  }
  int flags = socket_obj->fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    s3_log(S3_LOG_ERROR, "", "fcntl [cmd = F_GETFL] call failed: %s\n",
           strerror(errno));
    return -1;
  }
  if (socket_obj->fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
    s3_log(S3_LOG_ERROR, "", "fcntl [cmd = F_SETFL] call failed: %s\n",
           strerror(errno));
    return -1;
  }
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (socket_obj->inet_aton(host.c_str(), &server.sin_addr) == 0) {
    s3_log(S3_LOG_ERROR, "", "inet_aton call failed for host [%s]\n",
           host.c_str());
    errno = EINVAL;
    return -1;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return 0;
}

void S3StatsdStats::finish() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  if (sock != -1) {
    socket_obj->close(sock);
    sock = -1;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

int S3StatsdStats::send(const std::string& msg, int retry) {
  s3_log(S3_LOG_DEBUG, "", "msg: %s\n", msg.c_str());
  if (retry > g_option_instance->get_statsd_max_send_retry()) {
    retry = g_option_instance->get_statsd_max_send_retry();
  } else if (retry < 0) {
    retry = 0;
  }
  while (1) {
    if (socket_obj->sendto(sock, msg.c_str(), msg.length(), 0,
                           (struct sockaddr*)&server, sizeof(server)) == -1) {
      if ((errno == EAGAIN || errno == EWOULDBLOCK) && retry > 0) {
        retry--;
        continue;
      } else {
        s3_log(S3_LOG_ERROR, "", "sendto call failed: msg [%s], error [%s]\n",
               msg.c_str(), strerror(errno));
        return -1;
      }
    } else {
      break;
    }
  }
  return 0;
}

int S3StatsdStats::form_and_send_msg(const std::string& key,
                                     const std::string& type,
                                     const std::string& value, int retry,
                                     float sample_rate) {
  // validate key/value name
  bool isValid = is_keyname_valid(key);
  if (false == isValid)  // to avoid unused variable error
    assert(isValid);
  isValid = is_keyname_valid(value);
  assert(isValid);

  // check if metric is present in the allowlist
  if (!is_allowed_to_publish(key)) {
    s3_log(S3_LOG_DEBUG, "", "Metric not found in allowlist: [%s]\n",
           key.c_str());
    return 0;
  }

  // Form message in StatsD format:
  // <metricname>:<value>|<type>[|@<sampling rate>]
  std::string buf;
  if ((type == "s") || (type == "g")) {
    buf = key + ":" + value + "|" + type;
  } else if ((type == "c") || (type == "ms")) {
    if (is_fequal(sample_rate, 1.0)) {
      buf = key + ":" + value + "|" + type;
    } else {
      std::stringstream str_stream;
      str_stream << std::fixed << std::setprecision(2) << sample_rate;
      buf = key + ":" + value + "|" + type + "|@" + str_stream.str();
    }
  } else {
    s3_log(S3_LOG_ERROR, "", "Invalid type: key [%s], type [%s]\n", key.c_str(),
           type.c_str());
    errno = EINVAL;
    return -1;
  }

  return send(buf, retry);
}

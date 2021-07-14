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

#include <string>
#include <map>

#include "s3_log.h"
#include "s3_audit_info_logger_iem.h"
#include "s3_http_post_queue.h"

S3AuditInfoLoggerIEM::S3AuditInfoLoggerIEM(evbase_t* p_base,
                                           std::string host_ip, uint16_t port,
                                           std::string path) {
  s3_log(S3_LOG_INFO, nullptr, "%s Ctor\n", __func__);
  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "application/json";

  p_s3_post_queue.reset(create_http_post_queue(
      p_base, std::move(host_ip), port, std::move(path), std::move(headers)));
}

int S3AuditInfoLoggerIEM::save_msg(std::string const& eventID,
                                  std::string const& severity,
                                  std::string const& msg) {
  s3_log(S3_LOG_INFO, nullptr, "%s Entry", __func__);

  /*
  Sample JSON
  {
  "component": "S3",
  "source": "S",
  "module": "S3server",
  "event_id": "500",
  "severity": "B",
  "message_blob": "This is alert"
  }
  */
  std::string fmt_msg = std::string("{") +
                        std::string("\"component\": \"S3\",") +
                        std::string("\"source\": \"S\",") +
                        std::string("\"module\": \"S3server\",") +
                        std::string("\"event_id\": \"") + std::string(eventID) +
                        std::string("\",") + std::string("\"severity\": \"") +
                        std::string(severity) + std::string("\",") +
                        std::string("\"message_blob\": \"") + std::string(msg) +
                        std::string("\"") + std::string("}");

  s3_log(S3_LOG_DEBUG, nullptr, "IEM Message : %s", fmt_msg.c_str());
  const bool fSucc = p_s3_post_queue->post(std::move(fmt_msg));
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
  return !fSucc;
}

/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author:  Evgeniy Brazhnikov   <evgeniy.brazhnikov@seagate.com>
 * Author:  Evgeniy Brazhnikov   <evgeniy.brazhnikov@seagate.com>
 * Original creation date: 14-Jul-2020
 */
#include <algorithm>

#include "s3_log.h"
#include "s3_audit_info_logger_kafka_bridge.h"
#include "s3_http_post_queue.h"

S3AuditInfoLoggerKafkaBridge::S3AuditInfoLoggerKafkaBridge(evbase_t* p_base,
                                                           std::string host_ip,
                                                           uint16_t port,
                                                           std::string path) {
  s3_log(S3_LOG_INFO, nullptr, "Constructor");
  p_s3_post_queue.reset(
      new S3HttpPostQueue(p_base, std::move(host_ip), port, std::move(path)));
  p_s3_post_queue->add_header("Content-Type",
                              "application/vnd.kafka.json.v2+json");
}

S3AuditInfoLoggerKafkaBridge::~S3AuditInfoLoggerKafkaBridge() = default;

int S3AuditInfoLoggerKafkaBridge::save_msg(std::string const& request_id,
                                           std::string const& msg) {
  s3_log(S3_LOG_INFO, request_id, "Entering");
  s3_log(S3_LOG_DEBUG, request_id, "%s", msg.c_str());

  std::string fmt_msg =
      "{\"records\":[{\"key\":\"s3server\",\"value\":" + msg + "}]}";
  p_s3_post_queue->post(std::move(fmt_msg));

  s3_log(S3_LOG_DEBUG, request_id, "Exiting");
  return 0;
}

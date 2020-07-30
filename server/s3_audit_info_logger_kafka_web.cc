#include "s3_log.h"
#include "s3_audit_info_logger_kafka_web.h"
#include "s3_http_post_queue.h"

S3AuditInfoLoggerKafkaWeb::S3AuditInfoLoggerKafkaWeb(evbase_t* p_base,
                                                     std::string host_ip,
                                                     uint16_t port,
                                                     std::string path) {
  s3_log(S3_LOG_INFO, nullptr, "Constructor");
  p_s3_post_queue.reset(
      new S3HttpPostQueue(p_base, std::move(host_ip), port, std::move(path)));
  p_s3_post_queue->add_header("Content-Type",
                              "application/vnd.kafka.json.v2+json");
}

S3AuditInfoLoggerKafkaWeb::~S3AuditInfoLoggerKafkaWeb() = default;

int S3AuditInfoLoggerKafkaWeb::save_msg(std::string const& request_id,
                                        std::string const& msg) {
  s3_log(S3_LOG_INFO, request_id, "Entering");
  s3_log(S3_LOG_DEBUG, request_id, "%s", msg.c_str());

  std::string fmt_msg =
      "{\"records\":[{\"key\":\"s3server\",\"value\":" + msg + "}]}";
  const bool fSucc = p_s3_post_queue->post(std::move(fmt_msg));

  s3_log(S3_LOG_DEBUG, request_id, "Exiting");
  return !fSucc;
}

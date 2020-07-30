#pragma once

#ifndef __S3_SERVER_AUDIT_LOGGER_KAFKA_WEB__
#define __S3_SERVER_AUDIT_LOGGER_KAFKA_WEB__

#include <string>
#include <memory>

#include "s3_audit_info_logger_base.h"

class S3HttpPostQueue;

class S3AuditInfoLoggerKafkaWeb : public S3AuditInfoLoggerBase {
  typedef struct event_base evbase_t;

 public:
  S3AuditInfoLoggerKafkaWeb(evbase_t* p_base, std::string host_ip,
                            uint16_t port, std::string path);
  ~S3AuditInfoLoggerKafkaWeb();

  S3AuditInfoLoggerKafkaWeb(const S3AuditInfoLoggerKafkaWeb&) = delete;
  S3AuditInfoLoggerKafkaWeb& operator=(const S3AuditInfoLoggerKafkaWeb&) =
      delete;

  int save_msg(std::string const&, std::string const&) override;

 private:
  std::unique_ptr<S3HttpPostQueue> p_s3_post_queue;
};

#endif  // __S3_SERVER_AUDIT_LOGGER_KAFKA_WEB__

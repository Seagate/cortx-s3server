#pragma once

#ifndef __S3_SERVER_AUDIT_INFO_LOGGER_RSYSLOG_TCP_H__
#define __S3_SERVER_AUDIT_INFO_LOGGER_RSYSLOG_TCP_H__

#include "s3_audit_info_logger_base.h"
#include "s3_libevent_socket_wrapper.h"

class S3AuditInfoLoggerRsyslogTcp : public S3AuditInfoLoggerBase {
 public:
  S3AuditInfoLoggerRsyslogTcp(struct event_base *base,
                              std::string dest_host = "localhost",
                              int dest_port = 514, int retry_cnt = 5,
                              std::string message_id = "s3server-audit-logging",
                              int facility = 20,  // Rsyslog facility "local4"
                              int severity = 5,   // Rsyslog severity "notice"
                              std::string hostname = "-",  // no hostname
                              std::string app = "s3server",
                              std::string app_procid = "-",  // no procid
                              S3LibeventSocketWrapper *sock_api = nullptr);
  virtual ~S3AuditInfoLoggerRsyslogTcp();
  virtual int save_msg(std::string const &, std::string const &);

 private:
  std::string host;
  int port;
  int max_retry;
  int curr_retry;

  S3LibeventSocketWrapper *socket_api;
  bool managed_api;

  struct bufferevent *bev = nullptr;
  struct event_base *base_event = nullptr;
  struct evdns_base *base_dns = nullptr;

  std::string message_template;

  bool connecting = false;

  void connect();

 private:
  static void eventcb(struct bufferevent *bev, short events, void *ptr);
};

#endif

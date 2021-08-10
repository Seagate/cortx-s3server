package com.seagates3.model;

import com.seagates3.s3service.AuditLogRestClient;
import com.seagates3.authserver.AuthServerConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class AuthIAMAuditlog {

 private
  final static Logger LOGGER =
      LoggerFactory.getLogger(AuthIAMAuditlog.class.getName());

  // create json object
 public
  static void sendIAMAuditLog(String audit_logging_msg) {

    // rsyslog rfc5424 messge format
    // <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD MSG
    String message_id = AuthServerConfig.getRsyslogMsgId();
    int facility = 21;  // Rsyslog facility "local5"
    int severity = 5;   // Rsyslog severity "notice"
    String hostname = "localhost";
    String app = "S3AuthServer";
    String app_procid = "-";  // no procid

    String message_template = "<" + String.valueOf(facility * 8 + severity) +
                              ">1 " + "- " + hostname + " " + app + " " +
                              app_procid + " " + message_id + " " + "-";

    String auth_iam_audit_log = message_template + " " + audit_logging_msg;

    try {
      AuditLogRestClient.sendMessage(auth_iam_audit_log);
    }
    catch (Exception e) {
      LOGGER.error("IAM Audit log failed" + e.getMessage());
    }
  }
}

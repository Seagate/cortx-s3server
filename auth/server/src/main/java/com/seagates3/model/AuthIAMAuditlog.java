package com.seagates3.model;

import com.seagates3.s3service.AuditLogRestClient;
import com.seagates3.s3service.S3HttpResponse;

import com.amazonaws.util.json.JSONException;
import com.amazonaws.util.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class AuthIAMAuditlog {

 private
  final static Logger LOGGER =
      LoggerFactory.getLogger(AuthIAMAuditlog.class.getName());

  // create json object
 private
  static void saveAuthIAMAuditLog(String audit_logging_msg) {
    String auth_iam_audit_log = "";

    // rsyslog rfc5424 messge format
    // <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD MSG
    String message_id = "s3server-audit-logging";
    int facility = 21;  // Rsyslog facility "local5"
    int severity = 5;  // Rsyslog severity "notice"
    String hostname = "-";  // no hostname
    String app = "S3AuthServer";
    String app_procid = "-";  // no procid

    String message_template = "<" + String.valueOf(facility * 8 + severity) + ">1 " +
    "- " + hostname + " " + app + " " + app_procid + " " + message_id + " " + "-";

    auth_iam_audit_log = message_template + " " + audit_logging_msg;

    S3HttpResponse resp = new S3HttpResponse();

    try {
      resp = AuditLogRestClient.postRequest(auth_iam_audit_log);

      if (resp.getHttpCode() == 200) {
        LOGGER.debug("IAM Audit log successful");
      } else {
        LOGGER.error("IAM Audit log failed, status : " +
                     resp.getHttpCode());
      }
    }
    catch (Exception e) {
      LOGGER.error("IAM Audit log failed" + e.getMessage());
    }
  }
}

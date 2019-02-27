/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Siddhivinayak Shanbhag <siddhivinayak.shanbhag@seagate.com>
 * Original creation date: 27-February-2019
 */

#include "s3_audit_info.h"
#include "s3_log.h"
#include <sys/stat.h>
#include <string>
#include <stdio.h>

bool audit_configure_init(const std::string& log_config_path) {
  try {
    struct stat buf;
    if ((stat(log_config_path.c_str(), &buf) != -1) &&
        (!log_config_path.empty())) {
      PropertyConfigurator::configure(log_config_path);
      return true;
    } else {
      // BasicConfigurator log audit information to console.
      BasicConfigurator::configure();
      return false;
    }
  }
  catch (Exception&) {
    s3_log(S3_LOG_ERROR, "", "Exception occured while configuring audit logs.");
    return false;
  }
}

S3AuditInfo::S3AuditInfo() {
  bucket_owner_canonical_id = "-";
  bucket = "-";
  time_of_request_arrival = "-";
  remote_ip = "-";
  requester = "-";
  request_id = "-";
  operation = "-";
  object_key = "-";
  request_uri = "-";
  error_code = "-";
  bytes_sent = 0;
  object_size = "-";
  total_time = 0;
  turn_around_time = 0;
  referrer = "-";
  user_agent = "-";
  version_id = "-";
  host_id = "-";
  signature_version = "-";
  cipher_suite = "-";
  authentication_type = "-";
  host_header = "-";
}

const std::string& S3AuditInfo::format_audit_logger() {
  const std::string& formatted_log =
      "   " + bucket_owner_canonical_id + "   " + bucket + "   [" +
      time_of_request_arrival + "]   " + remote_ip + "   " + requester + "   " +
      request_id + "   " + operation + "   " + object_key + "   " +
      request_uri + "   " + std::to_string(http_status) + "   " + error_code +
      "   " + std::to_string(bytes_sent) + "   " + object_size + "   " +
      std::to_string(total_time) + "   " + std::to_string(turn_around_time) +
      "   " + referrer + "   " + user_agent + "   " + version_id + "   " +
      host_id + "   " + signature_version + "   " + cipher_suite + "   " +
      authentication_type + "   " + host_header;
  return formatted_log;
}

void S3AuditInfo::set_bucket_owner_canonical_id(
    const std::string& bucket_owner_canonical_id_str) {
  bucket_owner_canonical_id = bucket_owner_canonical_id_str;
}

void S3AuditInfo::set_bucket_name(const std::string& bucket_str) {
  bucket = bucket_str;
}

void S3AuditInfo::set_time_of_request_arrival(const std::string& time_str) {
  time_of_request_arrival = time_str;
}

void S3AuditInfo::set_remote_ip(const std::string& remote_ip_str) {
  remote_ip = remote_ip_str;
}

void S3AuditInfo::set_requester(const std::string& requester_str) {
  requester = requester_str;
}

void S3AuditInfo::set_request_id(const std::string& request_id_str) {
  request_id = request_id_str;
}

void S3AuditInfo::set_operation(const std::string& operation_str) {
  operation = operation_str;
}

void S3AuditInfo::set_object_key(const std::string& key_str) {
  object_key = key_str;
}

void S3AuditInfo::set_request_uri(const std::string& request_uri_str) {
  request_uri = request_uri_str;
}

void S3AuditInfo::set_http_status(int http_status_str) {
  http_status = http_status_str;
}

void S3AuditInfo::set_error_code(const std::string& error_code_str) {
  error_code = error_code_str;
}

void S3AuditInfo::set_bytes_sent(int bytes_sent_str) {
  bytes_sent = bytes_sent_str;
}

void S3AuditInfo::set_object_size(const std::string& object_size_str) {
  object_size = object_size_str;
}

void S3AuditInfo::set_total_time(size_t total_time_str) {
  total_time = total_time_str;
}

void S3AuditInfo::set_turn_around_time(size_t turn_around_time_str) {
  turn_around_time = turn_around_time_str;
}

void S3AuditInfo::set_referrer(const std::string& referrer_str) {
  referrer = referrer_str;
}

void S3AuditInfo::set_user_agent(const std::string& user_agent_str) {
  user_agent = user_agent_str;
}

void S3AuditInfo::set_version_id(const std::string& version_id_str) {
  version_id = version_id_str;
}

void S3AuditInfo::set_host_id(const std::string& host_id_str) {
  host_id = host_id_str;
}
void S3AuditInfo::set_signature_version(
    const std::string& signature_version_str) {
  signature_version = signature_version_str;
}
void S3AuditInfo::set_cipher_suite(const std::string& cipher_suite_str) {
  cipher_suite = cipher_suite_str;
}
void S3AuditInfo::set_authentication_type(
    const std::string& authentication_type_str) {
  authentication_type = authentication_type_str;
}
void S3AuditInfo::set_host_header(const std::string& host_header_str) {
  host_header = host_header_str;
}

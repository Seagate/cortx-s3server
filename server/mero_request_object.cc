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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-JULY-2019
 */

#include <evhttp.h>
#include <string>
#include <algorithm>

#include "s3_error_codes.h"
#include "s3_factory.h"
#include "s3_option.h"
#include "s3_common_utilities.h"
#include "mero_request_object.h"
#include "s3_stats.h"
#include "s3_audit_info_logger.h"

extern S3Option* g_option_instance;

MeroRequestObject::MeroRequestObject(
    evhtp_request_t* req, EvhtpInterface* evhtp_obj_ptr,
    std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buf_factory,
    EventInterface* event_obj_ptr)
    : RequestObject(req, evhtp_obj_ptr, async_buf_factory, event_obj_ptr),
      mero_api_type(MeroApiType::unsupported) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  // For auth disabled, use some dummy user.
  if (g_option_instance->is_auth_disabled()) {
    audit_log_obj.set_authentication_type("QueryString");
  } else {
    audit_log_obj.set_authentication_type("AuthHeader");
  }

  audit_log_obj.set_time_of_request_arrival();
}

MeroRequestObject::~MeroRequestObject() {
  s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");
  populate_and_log_audit_info();
}

void MeroRequestObject::set_key_name(const std::string& key) { key_name = key; }

const std::string& MeroRequestObject::get_key_name() { return key_name; }

void MeroRequestObject::set_object_oid_lo(const std::string& oid_lo) {
  object_oid_lo = oid_lo;
}

const std::string& MeroRequestObject::get_object_oid_lo() {
  return object_oid_lo;
}

void MeroRequestObject::set_object_oid_hi(const std::string& oid_hi) {
  object_oid_hi = oid_hi;
}

const std::string& MeroRequestObject::get_object_oid_hi() {
  return object_oid_hi;
}

void MeroRequestObject::set_index_id_lo(const std::string& index_lo) {
  index_id_lo = index_lo;
}

const std::string& MeroRequestObject::get_index_id_lo() { return index_id_lo; }

void MeroRequestObject::set_index_id_hi(const std::string& index_hi) {
  index_id_hi = index_hi;
}

const std::string& MeroRequestObject::get_index_id_hi() { return index_id_hi; }

void MeroRequestObject::set_api_type(MeroApiType api_type) {
  mero_api_type = api_type;
}

MeroApiType MeroRequestObject::get_api_type() { return mero_api_type; }

void MeroRequestObject::set_operation_code(MeroOperationCode operation_code) {
  mero_operation_code = operation_code;
}

MeroOperationCode MeroRequestObject::get_operation_code() {
  return mero_operation_code;
}

void MeroRequestObject::populate_and_log_audit_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering");
  if (S3Option::get_instance()->get_audit_logger_policy() == "disabled") {
    s3_log(S3_LOG_INFO, "", "Audit logger disabled by policy settings\n");
    return;
  }

  std::string mero_operation_str =
      operation_code_to_audit_str(get_operation_code());

  if (!(mero_operation_str.compare("NONE") &&
        mero_operation_str.compare("UNKNOWN"))) {
    mero_operation_str.clear();
  }

  std::string audit_operation_str =
      std::string("REST.") + get_http_verb_str(http_verb()) + std::string(".") +
      api_type_to_str(get_api_type()) + mero_operation_str;

  std::string request_uri =
      get_http_verb_str(http_verb()) + std::string(" ") + full_path_decoded_uri;

  if (!query_raw_decoded_uri.empty()) {
    request_uri = request_uri + std::string("?") + query_raw_decoded_uri;
  }

  request_uri = request_uri + std::string(" ") + http_version;

  audit_log_obj.set_turn_around_time(
      turn_around_time.elapsed_time_in_millisec());
  audit_log_obj.set_total_time(request_timer.elapsed_time_in_millisec());
  audit_log_obj.set_bytes_sent(bytes_sent);
  audit_log_obj.set_error_code(error_code_str);
  // audit_log_obj.set_object_size(length);

  audit_log_obj.set_bucket_owner_canonical_id(get_account_id());
  // audit_log_obj.set_bucket_name(bucket_name);
  audit_log_obj.set_remote_ip(get_header_value("X-Forwarded-For"));
  audit_log_obj.set_bytes_received(get_content_length());
  audit_log_obj.set_requester(get_account_id());
  audit_log_obj.set_request_id(request_id);
  audit_log_obj.set_operation(audit_operation_str);
  // audit_log_obj.set_object_key(c_get_full_path());
  audit_log_obj.set_request_uri(request_uri);
  audit_log_obj.set_http_status(http_status);
  audit_log_obj.set_signature_version(get_header_value("Authorization"));
  audit_log_obj.set_user_agent(get_header_value("User-Agent"));
  // audit_log_obj.set_version_id(get_query_string_value("versionId"));
  // Setting object size for PUT object request
  //   if (object_size != 0) {
  //     audit_log_obj.set_object_size(object_size);
  //   }
  audit_log_obj.set_host_header(get_header_value("Host"));

  if (S3AuditInfoLogger::save_msg(request_id, audit_log_obj.to_string()) < 0) {
    s3_log(S3_LOG_FATAL, request_id, "Audit Logger Error. STOP Server");
    exit(1);
  }
  s3_log(S3_LOG_DEBUG, request_id, "Exiting");
}

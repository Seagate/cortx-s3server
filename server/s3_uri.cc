/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original creation date: 1-Oct-2015
 */

#include <string>

#include "s3_log.h"
#include "s3_option.h"
#include "s3_uri.h"

S3URI::S3URI(std::shared_ptr<S3RequestObject> req)
    : request(req),
      operation_code(S3OperationCode::none),
      s3_api_type(S3ApiType::unsupported),
      bucket_name(""),
      object_name("") {
  request_id = request->get_request_id();
  setup_operation_code();
}

S3ApiType S3URI::get_s3_api_type() { return s3_api_type; }

std::string& S3URI::get_bucket_name() { return bucket_name; }

std::string& S3URI::get_object_name() { return object_name; }

S3OperationCode S3URI::get_operation_code() { return operation_code; }

void S3URI::setup_operation_code() {
  if (request->has_query_param_key("acl")) {
    operation_code = S3OperationCode::acl;
  } else if (request->has_query_param_key("policy")) {
    operation_code = S3OperationCode::policy;
  } else if (request->has_query_param_key("location")) {
    operation_code = S3OperationCode::location;
  } else if (request->has_query_param_key("uploads") ||
             request->has_query_param_key("uploadid")) {
    operation_code = S3OperationCode::multipart;
  } else if (request->has_query_param_key("delete")) {
    operation_code = S3OperationCode::multidelete;
  } else if (request->has_query_param_key("encryption")) {
    operation_code = S3OperationCode::encryption;
  } else if (request->has_query_param_key("requestPayment")) {
    operation_code = S3OperationCode::requestPayment;
  } else if (request->has_query_param_key("lifecycle")) {
    operation_code = S3OperationCode::lifecycle;
  } else if (request->has_query_param_key("cors")) {
    operation_code = S3OperationCode::cors;
  } else if (request->has_query_param_key("analytics")) {
    operation_code = S3OperationCode::analytics;
  } else if (request->has_query_param_key("inventory")) {
    operation_code = S3OperationCode::inventory;
  } else if (request->has_query_param_key("metrics")) {
    operation_code = S3OperationCode::metrics;
  } else if (request->has_query_param_key("tagging")) {
    operation_code = S3OperationCode::tagging;
  } else if (request->has_query_param_key("website")) {
    operation_code = S3OperationCode::website;
  } else if (request->has_query_param_key("replication")) {
    operation_code = S3OperationCode::replication;
  } else if (request->has_query_param_key("accelerate")) {
    operation_code = S3OperationCode::accelerate;
  } else if (request->has_query_param_key("logging")) {
    operation_code = S3OperationCode::logging;
  } else if (request->has_query_param_key("notification")) {
    operation_code = S3OperationCode::notification;
  } else if (request->has_query_param_key("torrent")) {
    operation_code = S3OperationCode::torrent;
  } else if (request->has_query_param_key("versioning")) {
    operation_code = S3OperationCode::versioning;
  } else if (request->has_query_param_key("versions")) {
    operation_code = S3OperationCode::versions;
  } else if (request->has_query_param_key("select")) {
    operation_code = S3OperationCode::selectcontent;
  } else if (request->has_query_param_key("restore")) {
    operation_code = S3OperationCode::restore;
  }

  s3_log(S3_LOG_DEBUG, request_id, "Operation code %s\n",
         operation_code_to_str(operation_code).c_str());
  // Other operations - todo
}

S3PathStyleURI::S3PathStyleURI(std::shared_ptr<S3RequestObject> req)
    : S3URI(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  std::string full_uri(request->c_get_full_path());
  // Strip the query params
  std::size_t qparam_start = full_uri.find("?");
  std::string full_path(full_uri, 0, qparam_start);
  // Regex is better, but lets live with raw parsing. regex = >gcc 4.9.0
  if (full_path.compare("/") == 0) {
    // FaultInjection request check
    std::string header_value =
        request->get_header_value("x-seagate-faultinjection");
    if (S3Option::get_instance()->is_fi_enabled() && !header_value.empty()) {
      s3_api_type = S3ApiType::faultinjection;
    } else {
      s3_api_type = S3ApiType::service;
    }
  } else if (request->get_header_value("x-seagate-mgmt-api") ==
             "true") {  // management api check check
    s3_log(S3_LOG_DEBUG, request_id, "x-seagate-mgmt-api is enabled.\n");
    s3_api_type = S3ApiType::management;
  } else {
    // Find the second forward slash.
    std::size_t pos =
        full_path.find("/", 1);  // ignoring the first forward slash
    if (pos == std::string::npos) {
      // no second slash, means only bucket name.
      bucket_name = std::string(full_path.c_str() + 1);
      s3_api_type = S3ApiType::bucket;
    } else if (pos == full_path.length() - 1) {
      // second slash and its last char, means only bucket name.
      bucket_name = std::string(full_path.c_str() + 1, full_path.length() - 2);
      s3_api_type = S3ApiType::bucket;
    } else {
      // Its an object api.
      s3_api_type = S3ApiType::object;
      bucket_name = std::string(full_path.c_str() + 1, pos - 1);
      object_name = std::string(full_path.c_str() + pos + 1);
      if (object_name.back() == '/') {
        object_name.pop_back();  // ignore last slash
      }
    }
  }
  request->set_api_type(s3_api_type);
}

S3VirtualHostStyleURI::S3VirtualHostStyleURI(
    std::shared_ptr<S3RequestObject> req)
    : S3URI(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  host_header = request->get_host_name();

  setup_bucket_name();

  std::string full_uri(request->c_get_full_path());
  // Strip the query params
  std::size_t qparam_start = full_uri.find("?");
  std::string full_path(full_uri, 0, qparam_start);
  if (full_path.compare("/") == 0) {
    s3_api_type = S3ApiType::bucket;
    request->set_api_type(s3_api_type);
  } else {
    s3_api_type = S3ApiType::object;
    request->set_api_type(s3_api_type);
    object_name = std::string(full_path.c_str() + 1);  // ignore first slash
    if (object_name.back() == '/') {
      object_name.pop_back();  // ignore last slash
    }
  }
}

void S3VirtualHostStyleURI::setup_bucket_name() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (host_header.find(S3Option::get_instance()->get_default_endpoint()) !=
      std::string::npos) {
    bucket_name = host_header.substr(
        0, (host_header.length() -
            S3Option::get_instance()->get_default_endpoint().length() - 1));
  }
  for (std::set<std::string>::iterator it =
           S3Option::get_instance()->get_region_endpoints().begin();
       it != S3Option::get_instance()->get_region_endpoints().end(); ++it) {
    if (host_header.find(*it) != std::string::npos) {
      bucket_name =
          host_header.substr(0, (host_header.length() - (*it).length() - 1));
    }
  }
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

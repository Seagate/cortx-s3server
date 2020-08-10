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
#include <evhttp.h>

#include "s3_log.h"
#include "s3_option.h"
#include "motr_uri.h"
#include "base64.h"

MotrURI::MotrURI(std::shared_ptr<MotrRequestObject> req)
    : request(req),
      operation_code(MotrOperationCode::none),
      motr_api_type(MotrApiType::unsupported) {
  request_id = request->get_request_id();
  setup_operation_code();
}

MotrApiType MotrURI::get_motr_api_type() { return motr_api_type; }

std::string& MotrURI::get_key_name() { return key_name; }
std::string& MotrURI::get_object_oid_lo() { return object_oid_lo; }
std::string& MotrURI::get_object_oid_hi() { return object_oid_hi; }
std::string& MotrURI::get_index_id_lo() { return index_id_lo; }
std::string& MotrURI::get_index_id_hi() { return index_id_hi; }

MotrOperationCode MotrURI::get_operation_code() { return operation_code; }

void MotrURI::setup_operation_code() {
  // Currently, operation codes are not defined for motr apis
  // so it is always 'none'
  operation_code = MotrOperationCode::none;
}

MotrPathStyleURI::MotrPathStyleURI(std::shared_ptr<MotrRequestObject> req)
    : MotrURI(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  std::string full_uri;
  const char* full_path = request->c_get_full_encoded_path();
  if (full_path) {
    full_uri = full_path;

    s3_log(S3_LOG_DEBUG, request_id, "Encoded request URI = %s\n", full_path);
    // Regex is better, but lets live with raw parsing. regex = >gcc 4.9.0
    if (full_uri.compare("/") == 0) {
      // FaultInjection request check
      std::string header_value =
          request->get_header_value("x-seagate-faultinjection");
      if (S3Option::get_instance()->is_fi_enabled() && !header_value.empty()) {
        motr_api_type = MotrApiType::faultinjection;
      }
    } else {
      // check for index operation on motr kvs
      std::string index_match = "/indexes/";
      std::string object_match = "/objects/";
      if (full_uri.find(index_match) == 0) {
        // ignoring the first '/indexes/' and search for next forward slash('/')
        std::size_t pos = full_uri.find("/", index_match.length());
        if ((pos == std::string::npos) || (pos == full_uri.length() - 1)) {
          // eg: /indexes/1234 or /indexes/1234/
          std::string index_str =
              full_uri.substr(index_match.length(), pos - index_match.length());
          // Find position of '-' delimeter in index string
          std::size_t delim_pos = index_str.find("-");
          if (delim_pos != std::string::npos) {
            char* decoded_index_id_hi = evhttp_uridecode(
                index_str.substr(0, delim_pos).c_str(), 1, NULL);
            index_id_hi = decoded_index_id_hi;
            s3_log(S3_LOG_DEBUG, request_id, "index_id_hi %s\n",
                   index_id_hi.c_str());
            free(decoded_index_id_hi);
            char* decoded_index_id_lo = evhttp_uridecode(
                index_str.substr(delim_pos + 1).c_str(), 1, NULL);
            index_id_lo = decoded_index_id_lo;
            s3_log(S3_LOG_DEBUG, request_id, "index_id_lo %s\n",
                   index_id_lo.c_str());
            free(decoded_index_id_lo);
            motr_api_type = MotrApiType::index;
          }
        } else {
          // get index id and key
          key_name = full_uri.substr(pos + 1);
          char* dec_key = evhttp_uridecode(key_name.c_str(), 1, NULL);
          if (dec_key) {
            key_name = dec_key;
            free(dec_key);
          }
          std::string index_str =
              full_uri.substr(index_match.length(), pos - index_match.length());
          // Find position of '-' delimeter in index string
          std::size_t delim_pos = index_str.find("-");
          if (delim_pos != std::string::npos) {
            char* decoded_index_id_hi = evhttp_uridecode(
                index_str.substr(0, delim_pos).c_str(), 1, NULL);
            index_id_hi = decoded_index_id_hi;
            s3_log(S3_LOG_DEBUG, request_id, "index_id_hi %s\n",
                   index_id_hi.c_str());
            free(decoded_index_id_hi);
            char* decoded_index_id_lo = evhttp_uridecode(
                index_str.substr(delim_pos + 1).c_str(), 1, NULL);
            index_id_lo = decoded_index_id_lo;
            s3_log(S3_LOG_DEBUG, request_id, "index_id_lo %s\n",
                   index_id_lo.c_str());
            free(decoded_index_id_lo);
            motr_api_type = MotrApiType::keyval;
          }
        }

      } else if (full_uri.find(object_match) == 0) {
        // check for object operation
        // ignoring the first '/objects/' and search for next forward slash('/')
        std::size_t pos = full_uri.find("/", object_match.length());
        if ((pos == std::string::npos) || (pos == full_uri.length() - 1)) {
          std::string object_oid_str = full_uri.substr(
              object_match.length(), pos - object_match.length());
          // Find position of '-' delimeter in object string
          std::size_t delim_pos = object_oid_str.find("-");
          if (delim_pos != std::string::npos) {

            char* decoded_object_oid_hi = evhttp_uridecode(
                object_oid_str.substr(0, delim_pos).c_str(), 1, NULL);
            object_oid_hi = decoded_object_oid_hi;
            s3_log(S3_LOG_DEBUG, request_id, "object_oid_hi %s\n",
                   object_oid_hi.c_str());
            free(decoded_object_oid_hi);
            char* decoded_object_oid_lo = evhttp_uridecode(
                object_oid_str.substr(delim_pos + 1).c_str(), 1, NULL);
            object_oid_lo = decoded_object_oid_lo;
            s3_log(S3_LOG_DEBUG, request_id, "object_oid_lo %s\n",
                   object_oid_lo.c_str());
            free(decoded_object_oid_lo);

            motr_api_type = MotrApiType::object;
          }
        }
      }
    }
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Empty Encoded request URI.\n");
  }

  request->set_api_type(motr_api_type);
}

// motr http URL Patterns for various APIs.

// list kv                -> http://s3.seagate.com/indexes/<indiex-id>
// get kv                 -> http://s3.seagate.com/indexes/<indiex-id>/<key>
// put kv                 -> http://s3.seagate.com/indexes/<indiex-id>/<key>
// delete kv              -> http://s3.seagate.com/indexes/<indiex-id>/<key>
// delete object oid      ->
// http://s3.seagate.com/objects/<object-oid>?layout-id=1

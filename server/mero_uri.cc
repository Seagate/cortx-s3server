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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-June-2019
 */

#include <string>

#include "s3_log.h"
#include "s3_option.h"
#include "mero_uri.h"

MeroURI::MeroURI(std::shared_ptr<MeroRequestObject> req)
    : request(req),
      operation_code(MeroOperationCode::none),
      mero_api_type(MeroApiType::unsupported) {
  request_id = request->get_request_id();
  setup_operation_code();
}

MeroApiType MeroURI::get_mero_api_type() { return mero_api_type; }

std::string& MeroURI::get_key_name() { return key_name; }
std::string& MeroURI::get_object_oid_lo() { return object_oid_lo; }
std::string& MeroURI::get_object_oid_hi() { return object_oid_hi; }
std::string& MeroURI::get_index_id_lo() { return index_id_lo; }
std::string& MeroURI::get_index_id_hi() { return index_id_hi; }

MeroOperationCode MeroURI::get_operation_code() { return operation_code; }

void MeroURI::setup_operation_code() {
  // Currently, operation codes are not defined for mero apis
  // so it is always 'none'
  operation_code = MeroOperationCode::none;
}

MeroPathStyleURI::MeroPathStyleURI(std::shared_ptr<MeroRequestObject> req)
    : MeroURI(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  std::string full_uri(request->c_get_full_path());
  // Regex is better, but lets live with raw parsing. regex = >gcc 4.9.0
  if (full_uri.compare("/") == 0) {
    // FaultInjection request check
    std::string header_value =
        request->get_header_value("x-seagate-faultinjection");
    if (S3Option::get_instance()->is_fi_enabled() && !header_value.empty()) {
      mero_api_type = MeroApiType::faultinjection;
    }
  } else {
    // check for index operation on mero kvs
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
          index_id_hi = index_str.substr(0, delim_pos);
          index_id_lo = index_str.substr(delim_pos + 1);
          mero_api_type = MeroApiType::index;
        }
      } else {
        // get index id and key
        key_name = full_uri.substr(pos + 1);
        std::string index_str =
            full_uri.substr(index_match.length(), pos - index_match.length());
        // Find position of '-' delimeter in index string
        std::size_t delim_pos = index_str.find("-");
        if (delim_pos != std::string::npos) {
          index_id_hi = index_str.substr(0, delim_pos);
          index_id_lo = index_str.substr(delim_pos + 1);
          mero_api_type = MeroApiType::keyval;
        }
      }

    } else if (full_uri.find(object_match) == 0) {
      // check for object operation
      // ignoring the first '/objects/' and search for next forward slash('/')
      std::size_t pos = full_uri.find("/", object_match.length());
      if ((pos == std::string::npos) || (pos == full_uri.length() - 1)) {
        std::string object_oid_str =
            full_uri.substr(index_match.length(), pos - index_match.length());
        // Find position of '-' delimeter in object string
        std::size_t delim_pos = object_oid_str.find("-");
        if (delim_pos != std::string::npos) {
          object_oid_hi = object_oid_str.substr(0, delim_pos);
          object_oid_lo = object_oid_str.substr(delim_pos + 1);
          mero_api_type = MeroApiType::object;
        }
      }
    }
  }

  request->set_api_type(mero_api_type);
}

// mero http URL Patterns for various APIs.

// list kv                -> http://s3.seagate.com/indexes/<indiex-id>
// get kv                 -> http://s3.seagate.com/indexes/<indiex-id>/<key>
// delete kv              -> http://s3.seagate.com/indexes/<indiex-id>/<key>
// delete object oid      ->
// http://s3.seagate.com/objects/<object-oid>?layout-id=1

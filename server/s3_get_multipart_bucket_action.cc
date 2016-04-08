/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Author         :  Rajesh Nambiar        <rajesh.nambiar@seagate.com>
 * Original creation date: 13-Jan-2016
 */

#include <string>

#include "s3_option.h"
#include "s3_get_multipart_bucket_action.h"
#include "s3_object_metadata.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetMultipartBucketAction::S3GetMultipartBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), return_list_size(0), fetch_successful(false), last_uploadid("") {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  request_marker_key = request->get_query_string_value("key-marker");
  if (!request_marker_key.empty()) {
    multipart_object_list.set_request_marker_key(request_marker_key);
  }
  s3_log(S3_LOG_DEBUG, "request_marker_key = %s\n", request_marker_key.c_str());

  last_key = request_marker_key;  // as requested by user

  request_marker_uploadid = request->get_query_string_value("upload-id-marker");
  multipart_object_list.set_request_marker_uploadid(request_marker_uploadid);
  s3_log(S3_LOG_DEBUG, "request_marker_uploadid = %s\n", request_marker_uploadid.c_str());
  last_uploadid = request_marker_uploadid;

  setup_steps();

  multipart_object_list.set_bucket_name(request->get_bucket_name());
  request_prefix = request->get_query_string_value("prefix");
  multipart_object_list.set_request_prefix(request_prefix);
  s3_log(S3_LOG_DEBUG, "prefix = %s\n", request_prefix.c_str());

  request_delimiter = request->get_query_string_value("delimiter");
  multipart_object_list.set_request_delimiter(request_delimiter);
  s3_log(S3_LOG_DEBUG, "delimiter = %s\n", request_delimiter.c_str());

  std::string maxuploads = request->get_query_string_value("max-uploads");
  if (maxuploads.empty()) {
    max_uploads = 1000;
    multipart_object_list.set_max_uploads("1000");
  } else {
    max_uploads = std::stoul(maxuploads);
    multipart_object_list.set_max_uploads(maxuploads);
  }
  s3_log(S3_LOG_DEBUG, "max-uploads = %s\n", maxuploads.c_str());
  // TODO request param validations
}

void S3GetMultipartBucketAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  if(!request_marker_uploadid.empty() && !request_marker_key.empty()) {
    add_task(std::bind( &S3GetMultipartBucketAction::get_key_object, this ));
  }
  add_task(std::bind( &S3GetMultipartBucketAction::get_next_objects, this ));
  add_task(std::bind( &S3GetMultipartBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetMultipartBucketAction::get_key_object() {
  s3_log(S3_LOG_DEBUG, "Fetching multipart listing\n");

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_multipart_bucket_index_name(), last_key, std::bind( &S3GetMultipartBucketAction::get_key_object_successful, this), std::bind( &S3GetMultipartBucketAction::get_key_object_failed, this));
}

void S3GetMultipartBucketAction::get_key_object_successful() {
  s3_log(S3_LOG_DEBUG, "Found list of multipart uploads\n");
  std::string key_name = last_key;
  if (!(clovis_kv_reader->get_value()).empty()) {
    s3_log(S3_LOG_DEBUG, "Read Object = %s\n", key_name.c_str());
    std::shared_ptr<S3ObjectMetadata> object = std::make_shared<S3ObjectMetadata>(request, true);
    size_t delimiter_pos = std::string::npos;
    std::string upload_str = object->get_upload_id();

    if (request_prefix.empty() && request_delimiter.empty()) {
      object->from_json(clovis_kv_reader->get_value());
      if(object->get_upload_id() > request_marker_uploadid)
        multipart_object_list.add_object(object);
    } else if (!request_prefix.empty() && request_delimiter.empty()) {
      // Filter out by prefix
      if (key_name.find(request_prefix) == 0) {
        object->from_json(clovis_kv_reader->get_value());
        if(object->get_upload_id() > request_marker_uploadid) {
          multipart_object_list.add_object(object);
        }
      }
    } else if (request_prefix.empty() && !request_delimiter.empty()) {
      delimiter_pos = key_name.find(request_delimiter);
      if (delimiter_pos == std::string::npos) {
        object->from_json(clovis_kv_reader->get_value());
        if(object->get_upload_id() > request_marker_uploadid) {
          multipart_object_list.add_object(object);
        }
      } else {
        // Roll up
        s3_log(S3_LOG_DEBUG, "Delimiter %s found at pos %zu in string %s\n", request_delimiter.c_str(), delimiter_pos, key_name.c_str());
        multipart_object_list.add_common_prefix(key_name.substr(0, delimiter_pos + 1));
      }
    } else {
      // both prefix and delimiter are not empty
      bool prefix_match = (key_name.find(request_prefix) == 0) ? true : false;
      if (prefix_match) {
        delimiter_pos = key_name.find(request_delimiter, request_prefix.length());
        if (delimiter_pos == std::string::npos) {
          object->from_json(clovis_kv_reader->get_value());
          if(object->get_upload_id() > request_marker_uploadid) {
            multipart_object_list.add_object(object);
          }
        } else {
          s3_log(S3_LOG_DEBUG, "Delimiter %s found at pos %zu in string %s\n", request_delimiter.c_str(), delimiter_pos, key_name.c_str());
          multipart_object_list.add_common_prefix(key_name.substr(0, delimiter_pos + 1));
        }
      } // else no prefix match, filter it out
    }

    return_list_size++;
    if (return_list_size == max_uploads) {
      // this is the last element returned or we reached limit requested
      last_key = key_name;
      last_uploadid = object->get_upload_id();
    }
  }

  if (return_list_size == max_uploads) {
    // Go ahead and respond.
    if (return_list_size == max_uploads) {
      multipart_object_list.set_response_is_truncated(true);
    }
    multipart_object_list.set_next_marker_key(last_key);
    multipart_object_list.set_next_marker_uploadid(last_uploadid);
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    next();
  }
}

void S3GetMultipartBucketAction::get_key_object_failed() {
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "No multipart uploads found\n");
    fetch_successful = true;  // With no entries.
    next();
  } else {
    s3_log(S3_LOG_DEBUG, "Failed to fetch multipart uploads listing\n");
    fetch_successful = false;
    send_response_to_s3_client();
  }
}

void S3GetMultipartBucketAction::get_next_objects() {
  s3_log(S3_LOG_DEBUG, "Fetching next set of multipart uploads listing\n");
  size_t count = S3Option::get_instance()->get_clovis_idx_fetch_count();

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->next_keyval(get_multipart_bucket_index_name(), last_key, count, std::bind( &S3GetMultipartBucketAction::get_next_objects_successful, this), std::bind( &S3GetMultipartBucketAction::get_next_objects_failed, this));
}

void S3GetMultipartBucketAction::get_next_objects_successful() {
  s3_log(S3_LOG_DEBUG, "Found multipart uploads listing\n");
  auto& kvps = clovis_kv_reader->get_key_values();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    s3_log(S3_LOG_DEBUG, "Read Object = %s\n", kv.first.c_str());
    auto object = std::make_shared<S3ObjectMetadata>(request, true);
    size_t delimiter_pos = std::string::npos;
    std::string upload_str = object->get_upload_id();

    if (request_prefix.empty() && request_delimiter.empty()) {
      object->from_json(kv.second);
      multipart_object_list.add_object(object);
    } else if (!request_prefix.empty() && request_delimiter.empty()) {
      // Filter out by prefix
      if (kv.first.find(request_prefix) == 0) {
        object->from_json(kv.second);
        multipart_object_list.add_object(object);
      }
    } else if (request_prefix.empty() && !request_delimiter.empty()) {
      delimiter_pos = kv.first.find(request_delimiter);
      if (delimiter_pos == std::string::npos) {
        object->from_json(kv.second);
        multipart_object_list.add_object(object);
      } else {
        // Roll up
        s3_log(S3_LOG_DEBUG, "Delimiter %s found at pos %zu in string %s\n", request_delimiter.c_str(), delimiter_pos, kv.first.c_str());
        multipart_object_list.add_common_prefix(kv.first.substr(0, delimiter_pos + 1));
      }
    } else {
      // both prefix and delimiter are not empty
      bool prefix_match = (kv.first.find(request_prefix) == 0) ? true : false;
      if (prefix_match) {
        delimiter_pos = kv.first.find(request_delimiter, request_prefix.length());
        if (delimiter_pos == std::string::npos) {
          object->from_json(kv.second);
          multipart_object_list.add_object(object);
        } else {
          s3_log(S3_LOG_DEBUG, "Delimiter %s found at pos %zu in string %s\n", request_delimiter.c_str(), delimiter_pos, kv.first.c_str());
          multipart_object_list.add_common_prefix(kv.first.substr(0, delimiter_pos + 1));
        }
      } // else no prefix match, filter it out
    }

    return_list_size++;
    if (--length == 0 || return_list_size == max_uploads) {
      // this is the last element returned or we reached limit requested
      last_key = kv.first;
      last_uploadid = object->get_upload_id();
      break;
    }
  }
  // We ask for more if there is any.
  size_t count_we_requested = S3Option::get_instance()->get_clovis_idx_fetch_count();

  if ((return_list_size == max_uploads) || (kvps.size() < count_we_requested)) {
    // Go ahead and respond.
    if (return_list_size == max_uploads) {
      multipart_object_list.set_response_is_truncated(true);
    }
    multipart_object_list.set_next_marker_key(last_key);
    multipart_object_list.set_next_marker_uploadid(last_uploadid);
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    get_next_objects();
  }
}

void S3GetMultipartBucketAction::get_next_objects_failed() {
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "No more multipart uploads listing\n");
    fetch_successful = true;  // With no entries.
  } else {
    s3_log(S3_LOG_DEBUG, "Failed to fetch multipart listing\n");
    fetch_successful = false;
  }
  send_response_to_s3_client();
}

void S3GetMultipartBucketAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger metadata read async operation with callback
  if (fetch_successful) {
    std::string& response_xml = multipart_object_list.get_multiupload_xml();

    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, "Object list response_xml = %s\n", response_xml.c_str());

    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

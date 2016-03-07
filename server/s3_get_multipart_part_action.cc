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

#include "s3_clovis_config.h"
#include "s3_get_multipart_part_action.h"
#include "s3_part_metadata.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetMultipartPartAction::S3GetMultipartPartAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), return_list_size(0), fetch_successful(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  request_marker_key = request->get_query_string_value("part-number-marker");
  multipart_part_list.set_request_marker_key(request_marker_key);
  s3_log(S3_LOG_DEBUG, "part-number-marker = %s\n", request_marker_key.c_str());

  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();
  last_key = request_marker_key;  // as requested by user
  upload_id = request->get_query_string_value("uploadId");
  multipart_part_list.set_bucket_name(bucket_name);
  multipart_part_list.set_object_name(object_name);
  multipart_part_list.set_upload_id(upload_id);
  multipart_part_list.set_storage_class("STANDARD");
  std::string maxparts = request->get_query_string_value("max-parts");
  if (maxparts.empty()) {
    max_parts = 1000;
    multipart_part_list.set_max_parts("1000");
  } else {
    max_parts = std::stoul(maxparts);
    multipart_part_list.set_max_parts(maxparts);
  }
  setup_steps();
  // TODO request param validations
}

void S3GetMultipartPartAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  if (!request_marker_key.empty()) {
    add_task(std::bind( &S3GetMultipartPartAction::get_key_object, this ));
  }
  add_task(std::bind( &S3GetMultipartPartAction::get_next_objects, this ));
  add_task(std::bind( &S3GetMultipartPartAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetMultipartPartAction::get_key_object() {
  s3_log(S3_LOG_DEBUG, "Fetching part listing\n");

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_part_index_name(), last_key, std::bind( &S3GetMultipartPartAction::get_key_object_successful, this), std::bind( &S3GetMultipartPartAction::get_key_object_failed, this));
}

void S3GetMultipartPartAction::get_key_object_successful() {
  s3_log(S3_LOG_DEBUG, "Found part listing\n");
  std::string key_name = last_key;
  if (!(clovis_kv_reader->get_value()).empty()) {
    s3_log(S3_LOG_DEBUG, "Read Part = %s\n", key_name.c_str());
    std::shared_ptr<S3PartMetadata> part = std::make_shared<S3PartMetadata>(request, upload_id, atoi(key_name.c_str()));

    part->from_json(clovis_kv_reader->get_value());

    return_list_size++;
    if (return_list_size == max_parts) {
      // this is the last element returned or we reached limit requested
      last_key = key_name;
    }
  }

  if (return_list_size == max_parts) {
    // Go ahead and respond.
    if (return_list_size == max_parts) {
      multipart_part_list.set_response_is_truncated(true);
      multipart_part_list.set_next_marker_key(last_key);
    }
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    next();
  }
}

void S3GetMultipartPartAction::get_key_object_failed() {
  s3_log(S3_LOG_DEBUG, "Failed to find part listing\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    fetch_successful = true;  // With no entries.
    next();
  } else {
    fetch_successful = false;
    send_response_to_s3_client();
  }
}

void S3GetMultipartPartAction::get_next_objects() {
  s3_log(S3_LOG_DEBUG, "Fetching next part listing\n");
  size_t count = S3ClovisConfig::get_instance()->get_clovis_idx_fetch_count();

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->next_keyval(get_part_index_name(), last_key, count, std::bind( &S3GetMultipartPartAction::get_next_objects_successful, this), std::bind( &S3GetMultipartPartAction::get_next_objects_failed, this));
}

void S3GetMultipartPartAction::get_next_objects_successful() {
  s3_log(S3_LOG_DEBUG, "Found part listing\n");
  auto& kvps = clovis_kv_reader->get_key_values();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    s3_log(S3_LOG_DEBUG, "Read Object = %s\n", kv.first.c_str());
    auto part = std::make_shared<S3PartMetadata>(request, upload_id, atoi(kv.first.c_str()));

    part->from_json(kv.second);
    multipart_part_list.add_part(part);

    return_list_size++;
    if (--length == 0 || return_list_size == max_parts) {
      // this is the last element returned or we reached limit requested
      last_key = kv.first;
      break;
    }
  }
  // We ask for more if there is any.
  size_t count_we_requested = S3ClovisConfig::get_instance()->get_clovis_idx_fetch_count();

  if ((return_list_size == max_parts) || (kvps.size() < count_we_requested)) {
    // Go ahead and respond.
    if (return_list_size == max_parts) {
      multipart_part_list.set_response_is_truncated(true);
      multipart_part_list.set_next_marker_key(last_key);
    }
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    get_next_objects();
  }
}

void S3GetMultipartPartAction::get_next_objects_failed() {
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "Missing part listing\n");
    fetch_successful = true;  // With no entries.
  } else {
    s3_log(S3_LOG_DEBUG, "Failed to find part listing\n");
    fetch_successful = false;
  }
  send_response_to_s3_client();
}

void S3GetMultipartPartAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger metadata read async operation with callback
  if (fetch_successful) {
    multipart_part_list.set_user_id(request->get_user_id());
    multipart_part_list.set_user_name(request->get_user_name());
    multipart_part_list.set_account_id(request->get_account_id());
    multipart_part_list.set_account_name(request->get_account_name());

    std::string& response_xml = multipart_part_list.get_multipart_xml();

    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, "Object list response_xml = %s\n", response_xml.c_str());

    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(), bucket_name);
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

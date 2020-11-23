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

#include "s3_copy_object_action.h"
#include "s3_log.h"
#include "s3_error_codes.h"
#include "s3_common_utilities.h"
#include "s3_motr_layout.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_m0_uint128_helper.h"

S3CopyObjectAction::S3CopyObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motrwriter_s3_factory,
    std::shared_ptr<S3MotrReaderFactory> motrreader_s3_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory)),
      write_in_progress(false),
      read_in_progress(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  s3_log(S3_LOG_INFO, request_id,
         "S3 API: CopyObject. Destination: [%s], Source: [%s]\n",
         request->get_object_uri().c_str(),
         request->get_copy_object_source().c_str());
  s3_copy_action_state = S3CopyObjectActionState::empty;

  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  new_object_oid = {0ULL, 0ULL};
  source_bucket_name = "";
  source_object_name = "";

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);
  // Note valid value is set during create object
  layout_id = -1;
  tried_count = 0;
  salt = "uri_salt_";

  if (motrwriter_s3_factory) {
    motr_writer_factory = std::move(motrwriter_s3_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }
  if (motrreader_s3_factory) {
    motr_reader_factory = std::move(motrreader_s3_factory);
  } else {
    motr_reader_factory = std::make_shared<S3MotrReaderFactory>();
  }
  setup_steps();
};

void S3CopyObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3CopyObjectAction::validate_copyobject_request, this);
  ACTION_TASK_ADD(S3CopyObjectAction::create_object, this);
  ACTION_TASK_ADD(S3CopyObjectAction::copy_object, this);
  ACTION_TASK_ADD(S3CopyObjectAction::save_metadata, this);
  ACTION_TASK_ADD(S3CopyObjectAction::send_response_to_s3_client, this);
}

void S3CopyObjectAction::get_source_bucket_and_object() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  std::string source = request->get_copy_object_source();
  size_t separator_pos = source.find("/");
  if (separator_pos != std::string::npos) {
    source_bucket_name = source.substr(0, separator_pos);
    source_object_name = source.substr(separator_pos + 1);
  }
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3CopyObjectAction::fetch_source_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Fetch metadata of bucket: %s\n",
         source_bucket_name.c_str());

  source_bucket_metadata = bucket_metadata_factory->create_bucket_metadata_obj(
      request, source_bucket_name);

  source_bucket_metadata->load(
      std::bind(&S3CopyObjectAction::fetch_source_bucket_info_success, this),
      std::bind(&S3CopyObjectAction::fetch_source_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3CopyObjectAction::fetch_source_bucket_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Found source bucket: [%s] metadata\n",
         source_bucket_name.c_str());

  // fetch source object metadata
  fetch_source_object_info();

  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3CopyObjectAction::fetch_source_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  s3_copy_action_state = S3CopyObjectActionState::validationFailed;

  if (source_bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Source bucket: [%s] not found\n",
           source_bucket_name.c_str());
    set_s3_error("NoSuchBucket");
  } else if (source_bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Source bucket metadata load operation failed due to pre launch "
           "failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Source bucket metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, request_id, "Exeting\n");
}

void S3CopyObjectAction::fetch_source_object_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Found source bucket metadata\n");
  m0_uint128 source_object_list_oid =
      source_bucket_metadata->get_object_list_index_oid();
  m0_uint128 source_object_version_list_oid =
      source_bucket_metadata->get_objects_version_list_index_oid();

  if ((source_object_list_oid.u_hi == 0ULL &&
       source_object_list_oid.u_lo == 0ULL) ||
      (source_object_version_list_oid.u_hi == 0ULL &&
       source_object_version_list_oid.u_lo == 0ULL)) {
    // Object list index and version list index missing.
    fetch_source_object_info_failed();
  } else {
    source_object_metadata =
        object_metadata_factory->create_object_metadata_obj(
            request, source_object_list_oid, source_bucket_name,
            source_object_name);

    source_object_metadata->set_objects_version_list_index_oid(
        source_object_version_list_oid);

    source_object_metadata->load(
        std::bind(&S3CopyObjectAction::fetch_source_object_info_success, this),
        std::bind(&S3CopyObjectAction::fetch_source_object_info_failed, this));
  }
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3CopyObjectAction::fetch_source_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "Successfully fetched source object metadata\n");

  if (MaxCopyObjectSourceSize < source_object_metadata->get_content_length()) {
    set_s3_error("InvalidRequest");
    send_response_to_s3_client();
  } else {
    next();
  }

  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void S3CopyObjectAction::fetch_source_object_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_copy_action_state = S3CopyObjectActionState::validationFailed;

  if (S3ObjectMetadataState::missing == source_object_metadata->get_state()) {
    set_s3_error("NoSuchKey");
  } else if (S3ObjectMetadataState::failed_to_launch ==
             source_object_metadata->get_state()) {
    s3_log(S3_LOG_ERROR, request_id,
           "Source object metadata load operation failed due to pre launch "
           "failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Source object metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, request_id, "Exeting\n");
}

// Validate source bucket and object
void S3CopyObjectAction::validate_copyobject_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  get_source_bucket_and_object();

  if (source_bucket_name.empty() || source_object_name.empty()) {
    set_s3_error("InvalidArgument");
    send_response_to_s3_client();
  } else {
    fetch_source_bucket_info();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// read source object
void S3CopyObjectAction::read_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// write to destination object
void S3CopyObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Destination bucket
void S3CopyObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  s3_copy_action_state = S3CopyObjectActionState::validationFailed;

  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Destination object
void S3CopyObjectAction::fetch_object_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Destination object
void S3CopyObjectAction::fetch_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Create destination object
void S3CopyObjectAction::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Copy source object to destination object
void S3CopyObjectAction::copy_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  read_object();              // read source object
  initiate_data_streaming();  // write to destination object
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Save destination object metadata
void S3CopyObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

std::string S3CopyObjectAction::get_response_xml() {
  std::string response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  response_xml +=
      "<CopyObjectResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
  response_xml += S3CommonUtilities::format_xml_string("LastModified", "");
  // ETag for the destination object would be same as Etag for Source Object
  response_xml += S3CommonUtilities::format_xml_string("ETag", "");
  response_xml += "</CopyObjectResult>";
  return response_xml;
}

void S3CopyObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (S3Option::get_instance()->is_getoid_enabled()) {

    request->set_out_header_value("x-stx-oid",
                                  S3M0Uint128Helper::to_string(new_object_oid));
    request->set_out_header_value("x-stx-layout-id", std::to_string(layout_id));
  }
  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    // Metadata saved for object is always a success condition.
    assert(s3_copy_action_state != S3CopyObjectActionState::metadataSaved);

    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code() == "ServiceUnavailable" ||
        get_s3_error_code() == "InternalError") {
      request->set_out_header_value("Connection", "close");
    }

    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }
    request->send_response(error.get_http_status_code(), response_xml);
  } else {
    std::string response_xml = get_response_xml();
    request->send_response(S3HttpSuccess200, response_xml);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

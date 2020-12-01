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

#include <cassert>
#include <algorithm>

#include "s3_copy_object_action.h"
#include "s3_log.h"
#include "s3_error_codes.h"
#include "s3_common_utilities.h"
#include "s3_motr_layout.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_m0_uint128_helper.h"
#include "s3_buffer_sequence.h"

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
         request->get_headers_copysource().c_str());
  s3_copy_action_state = S3CopyObjectActionState::empty;

  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  new_object_oid = {0ULL, 0ULL};

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
  std::string source = request->get_headers_copysource();
  size_t separator_pos = source.find("/");
  if (separator_pos != std::string::npos) {
    source_bucket_name = source.substr(0, separator_pos);
    source_object_name = source.substr(separator_pos + 1);
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
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

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::fetch_source_bucket_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Found source bucket: [%s] metadata\n",
         source_bucket_name.c_str());

  // fetch source object metadata
  fetch_source_object_info();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
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
            request, source_bucket_name, source_object_name,
            source_object_list_oid);

    source_object_metadata->set_objects_version_list_index_oid(
        source_object_version_list_oid);

    source_object_metadata->load(
        std::bind(&S3CopyObjectAction::fetch_source_object_info_success, this),
        std::bind(&S3CopyObjectAction::fetch_source_object_info_failed, this));
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::fetch_source_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "Successfully fetched source object metadata\n");

  if (MaxCopyObjectSourceSize < source_object_metadata->get_content_length()) {
    s3_copy_action_state = S3CopyObjectActionState::validationFailed;
    set_s3_error("InvalidRequest");
    send_response_to_s3_client();
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::fetch_source_object_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  s3_copy_action_state = S3CopyObjectActionState::validationFailed;

  m0_uint128 source_object_list_oid =
      source_bucket_metadata->get_object_list_index_oid();
  if (source_object_list_oid.u_hi == 0ULL &&
      source_object_list_oid.u_lo == 0ULL) {
    s3_log(S3_LOG_ERROR, request_id, "Object not found\n");
    set_s3_error("NoSuchKey");
  } else {
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
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Validate source bucket and object
void S3CopyObjectAction::validate_copyobject_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  get_source_bucket_and_object();

  if (source_bucket_name.empty() || source_object_name.empty()) {
    set_s3_error("InvalidArgument");
    send_response_to_s3_client();
  } else if (if_source_and_destination_same()) {
    s3_copy_action_state = S3CopyObjectActionState::validationFailed;
    set_s3_error("InvalidRequest");
    send_response_to_s3_client();
  } else {
    fetch_source_bucket_info();
  }
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

void S3CopyObjectAction::_set_layout_id(int layout_id) {
  assert(layout_id > 0 && layout_id < 15);
  this->layout_id = layout_id;

  motr_write_payload_size =
      S3Option::get_instance()->get_motr_write_payload_size(layout_id);
}

// Create destination object
void S3CopyObjectAction::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  content_length = source_object_metadata->get_content_length();
  _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
      content_length));
  motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layout_id);
  assert(motr_unit_size > 0);
  // TODO: handle tried_count

  if (!motr_writer) {
    motr_writer =
        motr_writer_factory->create_motr_writer(request, new_object_oid);
  } else {
    motr_writer->set_oid(new_object_oid);
  }
  motr_writer->create_object(
      std::bind(&S3CopyObjectAction::create_object_successful, this),
      std::bind(&S3CopyObjectAction::create_object_failed, this), layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::create_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering");
  s3_copy_action_state = S3CopyObjectActionState::newObjOidCreated;

  // New Object or overwrite, create new metadata and release old.
  new_object_metadata.reset(new S3ObjectMetadata(*source_object_metadata));

  new_object_metadata->set_object_list_index_oid(
      bucket_metadata->get_object_list_index_oid());

  new_object_metadata->set_objects_version_list_index_oid(
      bucket_metadata->get_objects_version_list_index_oid());

  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  // Generate a version id for the new object.
  new_object_metadata->regenerate_version_id();
  new_object_metadata->set_oid(motr_writer->get_oid());
  new_object_metadata->set_layout_id(layout_id);

  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::exists) {
    // collision_detected();
  } else {
    s3_copy_action_state = S3CopyObjectActionState::newObjOidCreationFailed;

    if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
      s3_log(S3_LOG_ERROR, request_id, "Create object failed.\n");
      set_s3_error("ServiceUnavailable");
    } else {
      s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
      // Any other error report failure.
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Copy source object to destination object
void S3CopyObjectAction::copy_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (!content_length) {
    s3_log(S3_LOG_DEBUG, request_id, "Source object is empty");
    next();
    return;
  }
  motr_reader = motr_reader_factory->create_motr_reader(
      request, source_object_metadata->get_oid(),
      source_object_metadata->get_layout_id());
  motr_reader->set_last_index(0);

  rest_bytes = content_length;
  read_data_block();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::read_data_block() {
  s3_log(S3_LOG_INFO, request_id, "Entering");

  assert(!read_in_progress);
  assert(rest_bytes > 0);

  const size_t max_blocks_in_one_read_op =
      S3Option::get_instance()->get_motr_units_per_request();
  const size_t rest_blocks = (rest_bytes + motr_unit_size - 1) / motr_unit_size;

  if (motr_reader->read_object_data(
          std::min(rest_blocks, max_blocks_in_one_read_op),
          std::bind(&S3CopyObjectAction::read_data_block_success, this),
          std::bind(&S3CopyObjectAction::read_data_block_failed, this))) {

    s3_log(S3_LOG_DEBUG, request_id, "Data read is launched");
    read_in_progress = true;
    return;
  }
  if (motr_reader->get_state() == S3MotrReaderOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
    s3_log(S3_LOG_ERROR, request_id,
           "read_object_data() failed due to motr_entity_open failure\n");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();

  s3_log(S3_LOG_DEBUG, NULL, "Exiting\n");
}

void S3CopyObjectAction::read_data_block_success() {
  assert(read_in_progress);

  s3_log(S3_LOG_INFO, request_id, "Read object data succeeded");
  read_in_progress = false;

  if (copy_failed) {
    send_response_to_s3_client();
    return;
  }
  if (!write_in_progress) {
    write_data_block();
  }
  s3_log(S3_LOG_DEBUG, NULL, "Exiting\n");
}

void S3CopyObjectAction::read_data_block_failed() {
  assert(read_in_progress);

  s3_log(S3_LOG_DEBUG, request_id, "Failed to read object data from motr");
  set_s3_error("InternalError");

  if (write_in_progress) {
    copy_failed = true;
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, NULL, "Exiting\n");
}

void S3CopyObjectAction::write_data_block() {
  s3_log(S3_LOG_INFO, request_id, "Entering");

  assert(!write_in_progress);
  assert(rest_bytes > 0);

  S3BufferSequence buffer_sequence;

  char* p_data;
  size_t block_size = motr_reader->get_first_block(&p_data);

  unsigned block_in_chunk = 0;
  size_t bytes_in_chunk = 0;

  while (block_size) {
    assert(p_data != NULL);
    assert(block_size && block_size <= (size_t)motr_unit_size);

    if (block_size >= rest_bytes) {
      block_size = rest_bytes;
      rest_bytes = 0;
    } else {
      rest_bytes -= block_size;
    }
    buffer_sequence.emplace_back(p_data, block_size);

    bytes_in_chunk += block_size;
    ++block_in_chunk;

    block_size = motr_reader->get_next_block(&p_data);
  }
  assert(block_in_chunk);
  assert(bytes_in_chunk);

  s3_log(S3_LOG_DEBUG, request_id, "Got %zu bytes in %u blocks", bytes_in_chunk,
         block_in_chunk);

  motr_writer->write_content(
      std::bind(&S3CopyObjectAction::write_data_block_success, this),
      std::bind(&S3CopyObjectAction::write_data_block_failed, this),
      std::move(buffer_sequence), motr_unit_size);

  write_in_progress = true;

  s3_log(S3_LOG_DEBUG, NULL, "Exiting\n");
}

void S3CopyObjectAction::write_data_block_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering");

  assert(write_in_progress);
  write_in_progress = false;

  if (rest_bytes) {
    read_data_block();
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, NULL, "Exiting\n");
}

void S3CopyObjectAction::write_data_block_failed() {
  assert(write_in_progress);

  s3_log(S3_LOG_INFO, request_id, "Failed to write object data to motr");
  set_s3_error("InternalError");

  if (read_in_progress) {
    copy_failed = true;
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, NULL, "Exiting\n");
}

void S3CopyObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  std::string s_md5_got = source_object_metadata->get_md5();

  if (!s_md5_got.empty()) {
    std::string s_md5_calc = motr_writer->get_content_md5();
    s3_log(S3_LOG_DEBUG, request_id, "MD5 calculated: %s, MD5 got %s",
           s_md5_calc.c_str(), s_md5_got.c_str());

    if (s_md5_calc != s_md5_got) {
      s3_log(S3_LOG_ERROR, request_id, "Content MD5 mismatch\n");
      s3_copy_action_state = S3CopyObjectActionState::md5ValidationFailed;

      set_s3_error("BadDigest");
      // Clean up will be done after response.
      send_response_to_s3_client();
      return;
    }
  }
  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");

  new_object_metadata->reset_date_time_to_current();
  new_object_metadata->set_content_length(std::to_string(content_length));
  new_object_metadata->set_content_type(
      source_object_metadata->get_content_type());
  new_object_metadata->set_md5(motr_writer->get_content_md5());
  // new_object_metadata->set_tags(new_object_tags_map);

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Writing user metadata on object: [%s] -> [%s]\n",
             it.first.c_str(), it.second.c_str());
      new_object_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }

  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  new_object_metadata->save(
      std::bind(&S3CopyObjectAction::save_object_metadata_success, this),
      std::bind(&S3CopyObjectAction::save_object_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_copy_action_state = S3CopyObjectActionState::metadataSaved;
  next();
}

void S3CopyObjectAction::save_object_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_copy_action_state = S3CopyObjectActionState::metadataSaveFailed;
  if (new_object_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "failed to save object metadata.");
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
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

    S3Error error(get_s3_error_code(), request->get_request_id());

    if (S3CopyObjectActionState::validationFailed == s3_copy_action_state &&
        "InvalidRequest" == get_s3_error_code()) {
      if (if_source_and_destination_same()) {  // Source and Destination same
        error.set_auth_error_message(InvalidRequestSourceAndDestinationSame);
      } else if (source_object_metadata->get_content_length() >
                 MaxCopyObjectSourceSize) {  // Source object size greater than
                                             // 5GB
        error.set_auth_error_message(
            InvalidRequestSourceObjectSizeGreaterThan5GB);
      }
    }
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

bool S3CopyObjectAction::if_source_and_destination_same() {
  return ((source_bucket_name == request->get_bucket_name()) &&
          (source_object_name == request->get_object_name()));
}

void S3CopyObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

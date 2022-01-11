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
#include <utility>

#include "s3_common.h"
#include "s3_common_utilities.h"
#include "s3_put_multiobject_copy_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_perf_metrics.h"
#include "s3_stats.h"
#include "s3_motr_layout.h"
#include "s3_m0_uint128_helper.h"
#include "s3_probable_delete_record.h"
#include "s3_uri_to_motr_oid.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3PutMultipartCopyAction::S3PutMultipartCopyAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory,
    std::shared_ptr<S3MotrReaderFactory> motr_s3_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<S3AuthClientFactory> auth_factory)
    : S3PutObjectActionBase(std::move(req), std::move(bucket_meta_factory),
                            std::move(object_meta_factory), std::move(motr_api),
                            std::move(motr_s3_writer_factory),
                            std::move(kv_writer_factory)) {
  part_number = get_part_number();
  upload_id = request->get_query_string_value("uploadId");
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: UploadPartCopy. Destination: [%s], Source: [%s], \
         Part[%d] for UploadId [%s]\n",
         request->get_object_uri().c_str(),
         request->get_headers_copysource().c_str(), part_number,
         upload_id.c_str());

  action_uses_cleanup = true;
  s3_copy_part_action_state = S3PutObjectActionState::empty;
  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  layout_id = -1;  // Will be set during create object
  new_object_oid = {0ULL, 0ULL};

  if (S3Option::get_instance()->is_auth_disabled()) {
    auth_completed = true;
  }

  if (object_mp_meta_factory) {
    object_mp_metadata_factory = object_mp_meta_factory;
  } else {
    object_mp_metadata_factory =
        std::make_shared<S3ObjectMultipartMetadataFactory>();
  }

  if (part_meta_factory) {
    part_metadata_factory = part_meta_factory;
  } else {
    part_metadata_factory = std::make_shared<S3PartMetadataFactory>();
  }

  if (motr_s3_writer_factory) {
    motr_writer_factory = motr_s3_writer_factory;
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }

  if (kv_writer_factory) {
    motr_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    motr_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  if (motr_s3_reader_factory) {
    motr_reader_factory = std::move(motr_s3_reader_factory);
  } else {
    motr_reader_factory = std::make_shared<S3MotrReaderFactory>();
  }

  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);

  setup_steps();
}

void S3PutMultipartCopyAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3PutMultipartCopyAction::validate_multipart_partcopy_request,
                  this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::check_part_details, this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::fetch_multipart_metadata, this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::fetch_part_info, this);
  ACTION_TASK_ADD(
      S3PutMultipartCopyAction::set_source_bucket_authorization_metadata, this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::check_source_bucket_authorization,
                  this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::create_part, this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::initiate_part_copy, this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::save_metadata, this);
  ACTION_TASK_ADD(S3PutMultipartCopyAction::send_response_to_s3_client, this);
}

// Validate source bucket and object
void S3PutMultipartCopyAction::validate_multipart_partcopy_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (additional_bucket_name.empty() || additional_object_name.empty()) {
    set_s3_error("InvalidArgument");
    send_response_to_s3_client();
    return;
  } else if (if_source_and_destination_same()) {
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidRequest");
    send_response_to_s3_client();
    return;
  }
  std::string range_header_value =
      request->get_header_value("x-amz-copy-source-range");
  source_object_size = additional_object_metadata->get_content_length();

  if (range_header_value.empty()) {
    // Range is not specified, read complete object
    s3_log(S3_LOG_DEBUG, request_id, "Range is not specified\n");
    if (MaxPartCopySourcePartSize < source_object_size) {
      s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
      set_s3_error("InvalidRequest");
      send_response_to_s3_client();
      return;
    } else {
      last_byte_offset_to_copy = source_object_size - 1;
    }
  } else {
    // parse the Range header value
    // eg: bytes=0-1024 value
    s3_log(S3_LOG_DEBUG, request_id, "Range found(%s)\n",
           range_header_value.c_str());
    if (!validate_range_header(range_header_value)) {
      set_s3_error("InvalidRange");
      send_response_to_s3_client();
      return;
    } else {
      is_range_copy = true;
    }
  }
  total_data_to_copy = last_byte_offset_to_copy - first_byte_offset_to_copy;
  s3_log(S3_LOG_DEBUG, request_id, "Validated range: %zu-%zu\n",
         first_byte_offset_to_copy, last_byte_offset_to_copy);
  s3_log(S3_LOG_DEBUG, request_id,
         "Total data to copy in part [%d]: %zu bytes\n", part_number,
         total_data_to_copy);
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

bool S3PutMultipartCopyAction::validate_range_header(
    const std::string& range_value) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // The header can consist of 'blank' character(s) only
  if (std::find_if_not(range_value.begin(), range_value.end(), &::isspace) ==
      range_value.end()) {
    s3_log(S3_LOG_DEBUG, request_id,
           "Range header consists of blank symbol(s) only");
    return true;
  }
  // parse the Range header value
  // eg: bytes=0-1024 value
  std::size_t pos = range_value.find('=');
  // return false when '=' not found
  if (pos == std::string::npos) {
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }

  std::string bytes_unit = S3CommonUtilities::trim(range_value.substr(0, pos));
  std::string byte_range_set = range_value.substr(pos + 1);

  // check bytes_unit has bytes string or not
  if (bytes_unit != "bytes") {
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }

  if (byte_range_set.empty()) {
    // found range as bytes=
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }
  // byte_range_set has multi range
  pos = byte_range_set.find(',');
  if (pos != std::string::npos) {
    // found ,
    s3_log(S3_LOG_INFO, stripped_request_id, "unsupported multirange(%s)\n",
           byte_range_set.c_str());
    return false;
  }
  pos = byte_range_set.find('-');
  if (pos == std::string::npos) {
    // not found -
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }

  std::string first_byte = byte_range_set.substr(0, pos);
  std::string last_byte = byte_range_set.substr(pos + 1);

  // trip leading and trailing space
  first_byte = S3CommonUtilities::trim(first_byte);
  last_byte = S3CommonUtilities::trim(last_byte);

  // invalid pre-condition checks
  // 1. first and last byte offsets are empty
  // 2. first/last byte is not empty and it has invalid data like char
  if ((first_byte.empty() && last_byte.empty()) ||
      (!first_byte.empty() &&
       !S3CommonUtilities::string_has_only_digits(first_byte)) ||
      (!last_byte.empty() &&
       !S3CommonUtilities::string_has_only_digits(last_byte))) {
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }

  first_byte_offset_to_copy = strtol(first_byte.c_str(), 0, 10);
  last_byte_offset_to_copy = strtol(last_byte.c_str(), 0, 10);

  if ((first_byte_offset_to_copy >= source_object_size) ||
      (first_byte_offset_to_copy > last_byte_offset_to_copy)) {
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return true;
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::check_part_details() {
  // "Part numbers can be any number from 1 to 10,000, inclusive."
  // https://docs.aws.amazon.com/AmazonS3/latest/API/API_UploadPartCopy.html
  if (part_number < MINIMUM_PART_NUMBER || part_number > MAXIMUM_PART_NUMBER) {
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidPart");
    send_response_to_s3_client();
  } else if (request->get_header_size() > MAX_HEADER_SIZE ||
             request->get_user_metadata_size() > MAX_USER_METADATA_SIZE) {
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("MetadataTooLarge");
    send_response_to_s3_client();
  } else if ((request->get_object_name()).length() > MAX_OBJECT_KEY_LENGTH) {
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("KeyTooLongError");
    send_response_to_s3_client();
  } else {
    next();
  }
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::fetch_multipart_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string multipart_key_name =
      request->get_object_name() + EXTENDED_METADATA_OBJECT_SEP + upload_id;
  object_multipart_metadata =
      object_mp_metadata_factory->create_object_mp_metadata_obj(
          request, bucket_metadata->get_multipart_index_layout(), upload_id);

  // In multipart table key is object_name + |uploadid
  object_multipart_metadata->rename_object_name(multipart_key_name);

  object_multipart_metadata->load(
      std::bind(&S3PutMultipartCopyAction::next, this),
      std::bind(&S3PutMultipartCopyAction::fetch_multipart_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::fetch_multipart_failed() {
  // Log error
  s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
  s3_log(S3_LOG_ERROR, request_id,
         "Failed to retrieve multipart upload metadata\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::missing) {
    set_s3_error("NoSuchUpload");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::fetch_part_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  part_metadata = part_metadata_factory->create_part_metadata_obj(
      request, object_multipart_metadata->get_part_index_layout(), upload_id,
      part_number);

  part_metadata->load(
      std::bind(&S3PutMultipartCopyAction::fetch_part_info_success, this),
      std::bind(&S3PutMultipartCopyAction::fetch_part_info_failed, this),
      part_number);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::fetch_part_info_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata->get_state() == S3PartMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id,
           "S3PartMetadataState::present for part %d\n", part_number);
    old_object_oid = part_metadata->get_oid();
    old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
    old_layout_id = part_metadata->get_layout_id();
    next();
  } else if (part_metadata->get_state() == S3PartMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "S3PartMetadataState::missing\n");
    next();
  } else if (part_metadata->get_state() ==
             S3PartMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Part metadata load operation failed due to pre launch failure\n");
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to look up metadata.\n");
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::fetch_part_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (part_metadata->get_state() == S3PartMetadataState::missing) {
    next();
  } else {
    s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

bool S3PutMultipartCopyAction::copy_part_object_cb() {
  if (check_shutdown_and_rollback() || !request->client_connected()) {
    return true;
  }
  if (response_started) {
    request->send_reply_body(xml_spaces, sizeof(xml_spaces) - 1);
  }
  return false;
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::create_part() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_timer.start();
  motr_writer = motr_writer_factory->create_motr_writer(
      request, new_object_oid, object_multipart_metadata->get_pvid(), 0);
  // TODO: Handle target part size with CopySourceRange
  _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
      request->get_content_length()));
  motr_writer->set_layout_id(layout_id);

  motr_writer->create_object(
      std::bind(&S3PutMultipartCopyAction::create_part_object_successful, this),
      std::bind(&S3PutMultipartCopyAction::create_part_object_failed, this),
      new_object_oid, layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_multiobject_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::create_part_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_copy_part_action_state = S3PutObjectActionState::newObjOidCreated;

  part_metadata = part_metadata_factory->create_part_metadata_obj(
      request, object_multipart_metadata->get_part_index_layout(), upload_id,
      part_number);
  part_metadata->set_oid(motr_writer->get_oid());
  part_metadata->set_layout_id(layout_id);
  part_metadata->set_pvid(motr_writer->get_ppvid());
  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  add_object_oid_to_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::create_part_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  s3_timer.stop();
  const auto mss = s3_timer.elapsed_time_in_millisec();
  LOG_PERF("create_part_object_failed_ms", request_id.c_str(), mss);
  s3_stats_timing("create_part_object_failed", mss);

  s3_copy_part_action_state = S3PutObjectActionState::newObjOidCreationFailed;

  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Create object failed.\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
    // Any other error report failure.
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Initiate copy of source part object to the destination
void S3PutMultipartCopyAction::initiate_part_copy() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (additional_object_metadata->get_number_of_fragments() == 0) {
    copy_part_object();
  } else {
    copy_multipart_source_object();
  }
}

void S3PutMultipartCopyAction::copy_multipart_source_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (!total_data_to_copy) {
    s3_log(S3_LOG_DEBUG, stripped_request_id, "Source object is empty");
    next();
    return;
  }
  object_data_copier.reset(new S3ObjectDataCopier(
      request, motr_writer, motr_reader_factory, s3_motr_api));

  extended_obj = additional_object_metadata->get_extended_object_metadata();

  total_objects = additional_object_metadata->get_number_of_fragments();

  extended_obj = additional_object_metadata->get_extended_object_metadata();
  const std::map<int, std::vector<struct s3_part_frag_context>>& ext_entries =
      extended_obj->get_raw_extended_entries();
  for (unsigned int i = 0; i < total_objects; i++) {
    int ext_entry_index = 0;
    const struct s3_part_frag_context& frag_info =
        (ext_entries.at(i + 1)).at(ext_entry_index);
    if (is_range_copy) {
      struct S3ExtendedObjectInfo obj_info;
      if (i == 0) {
        // first part object
        obj_info.start_offset_in_object = 0;
      } else {
        obj_info.start_offset_in_object =
            extended_objects[i - 1].start_offset_in_object +
            extended_objects[i - 1].object_size;
      }
      obj_info.object_OID = frag_info.motr_OID;
      obj_info.object_layout = frag_info.layout_id;
      obj_info.object_pvid = frag_info.PVID;
      obj_info.object_size = frag_info.item_size;
      obj_info.requested_object_size = obj_info.object_size;
      size_t motr_unit_size =
          S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
              obj_info.object_layout);
      /* Count Data blocks from data size */
      obj_info.total_blocks_in_object =
          (obj_info.object_size + (motr_unit_size - 1)) / motr_unit_size;
      total_blocks_in_object += obj_info.total_blocks_in_object;
      s3_log(S3_LOG_DEBUG, request_id,
             "motr_unit_size = %zu for layout_id = %d in object (oid) ="
             "%" SCNx64 " : %" SCNx64 " with blocks in object = (%zu)\n",
             motr_unit_size, obj_info.object_layout, obj_info.object_OID.u_hi,
             obj_info.object_OID.u_lo, obj_info.total_blocks_in_object);
      extended_objects.push_back(obj_info);
    }
    part_fragment_context_list.push_back(frag_info);
  }
  bool f_success = false;
  if (is_range_copy) set_range_read_from_source_multipart_object();
  try {
    object_data_copier->copy_part_fragment_in_single_source(
        part_fragment_context_list, extended_objects, first_byte_offset_to_copy,
        std::bind(&S3PutMultipartCopyAction::copy_part_object_cb, this),
        std::bind(&S3PutMultipartCopyAction::copy_part_object_success, this),
        std::bind(&S3PutMultipartCopyAction::copy_part_object_failed, this),
        is_range_copy);
    f_success = true;
  }
  catch (const std::exception& ex) {
    s3_log(S3_LOG_ERROR, stripped_request_id, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, stripped_request_id, "Non-standard C++ exception");
  }
  if (!f_success) {
    object_data_copier.reset();

    set_s3_error("InternalError");
    send_response_to_s3_client();
    return;
  }
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit\n", __func__);
}

void S3PutMultipartCopyAction::set_range_read_from_source_multipart_object() {
  unsigned int first_byte_object_index = 0,
               last_byte_object_index = total_objects - 1;
  total_objects_to_copy = total_objects;
  for (unsigned int i = 0; i < total_objects; i++) {
    if (first_byte_offset_to_copy <
        (extended_objects[i].start_offset_in_object +
         extended_objects[i].object_size)) {
      // Found first_byte_offset_to_copy in object at index i
      first_byte_object_index = i;
      break;
    }
  }
  for (unsigned int j = first_byte_object_index; j < total_objects; j++) {
    if (last_byte_offset_to_copy < (extended_objects[j].start_offset_in_object +
                                    extended_objects[j].object_size)) {
      // Found last_byte_offset_to_copy in object at index i
      last_byte_object_index = j;
      break;
    }
  }
  // Calculate the actual requested content in first object
  // (first_byte_object_index) and last object (last_byte_object_index)
  if (last_byte_object_index == first_byte_object_index) {
    // If both first byte offset and last byte offset are in same object
    extended_objects[first_byte_object_index].requested_object_size =
        last_byte_offset_to_copy - first_byte_offset_to_copy + 1;
  } else {
    extended_objects[first_byte_object_index].requested_object_size =
        (extended_objects[first_byte_object_index].object_size -
         (first_byte_offset_to_copy -
          extended_objects[first_byte_object_index].start_offset_in_object));

    extended_objects[last_byte_object_index].requested_object_size =
        last_byte_offset_to_copy -
        (extended_objects[last_byte_object_index].start_offset_in_object);
  }

  // Get the block of first_byte_offset_to_copy from object at
  // index first_byte_object_index
  size_t unit_size_of_object_with_first_byte = 0,
         unit_size_of_object_with_last_byte = 0;
  unit_size_of_object_with_first_byte =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
          extended_objects[first_byte_object_index].object_layout);
  if (last_byte_object_index != first_byte_object_index) {
    unit_size_of_object_with_last_byte =
        S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
            extended_objects[last_byte_object_index].object_layout);
  } else {
    // Both offsets belong to same object
    unit_size_of_object_with_last_byte = unit_size_of_object_with_first_byte;
  }

  size_t first_byte_offset_block =
      (first_byte_offset_to_copy -
       extended_objects[first_byte_object_index].start_offset_in_object +
       unit_size_of_object_with_first_byte) /
      unit_size_of_object_with_first_byte;

  // Get the block of last_byte_offset_to_copy from object at
  // index 'last_byte_object_index'
  size_t last_byte_offset_block =
      (last_byte_offset_to_copy -
       extended_objects[last_byte_object_index].start_offset_in_object +
       unit_size_of_object_with_last_byte) /
      unit_size_of_object_with_last_byte;

  // Get total number blocks to read for a given valid range
  for (unsigned int k = first_byte_object_index; k <= last_byte_object_index;
       k++) {
    // Calculate blocks in each object in the valid object range with offsets
    if (k == first_byte_object_index) {
      total_blocks_to_copy_all_objects +=
          extended_objects[k].total_blocks_in_object - first_byte_offset_block +
          1;
    } else if (k == last_byte_object_index) {
      total_blocks_to_copy_all_objects += last_byte_offset_block;
    } else {
      total_blocks_to_copy_all_objects +=
          extended_objects[k].total_blocks_in_object;
    }
  }
  // Check if 'extended_objects' needs to be shrunk, due to specified byte
  // range
  bool need_to_shrink_front = false, need_to_shrink_end = false;
  // First, check for first byte offset object
  if (first_byte_object_index != 0 && first_byte_object_index < total_objects) {
    // First byte to read is not in first object, so shrink the array of
    // objects
    need_to_shrink_front = true;
  }
  // Second, check for last byte offset object
  if (last_byte_object_index != (total_objects - 1) &&
      last_byte_object_index < total_objects) {
    // Last byte to read is not in the last object, so shrink the array of
    // objects
    need_to_shrink_end = true;
  }
  std::vector<struct S3ExtendedObjectInfo> new_set_of_extended_objects;
  std::vector<struct s3_part_frag_context> new_part_fragment_context_list;
  if (need_to_shrink_front || need_to_shrink_end) {
    if (need_to_shrink_front && need_to_shrink_end) {
      std::copy(extended_objects.begin() + first_byte_object_index,
                (extended_objects.begin() + last_byte_object_index + 1),
                back_inserter(new_set_of_extended_objects));
      std::copy(
          part_fragment_context_list.begin() + first_byte_object_index,
          (part_fragment_context_list.begin() + last_byte_object_index + 1),
          back_inserter(new_part_fragment_context_list));
    } else if (need_to_shrink_front) {
      // Shrink front only
      std::copy(extended_objects.begin() + first_byte_object_index,
                extended_objects.end(),
                back_inserter(new_set_of_extended_objects));
      std::copy(part_fragment_context_list.begin() + first_byte_object_index,
                part_fragment_context_list.end(),
                back_inserter(new_part_fragment_context_list));
    } else {
      // Shrink end only
      std::copy(extended_objects.begin(),
                (extended_objects.begin() + last_byte_object_index + 1),
                back_inserter(new_set_of_extended_objects));
      std::copy(
          part_fragment_context_list.begin(),
          (part_fragment_context_list.begin() + last_byte_object_index + 1),
          back_inserter(new_part_fragment_context_list));
    }
    // Re-set the array of objects to read data from
    part_fragment_context_list = new_part_fragment_context_list;
    extended_objects = new_set_of_extended_objects;
    // Re-calculate total objects to read data from
    total_objects_to_copy = extended_objects.size();
  }

  s3_log(S3_LOG_DEBUG, request_id,
         "total_objects_to_copy: (%zu)\n,total_blocks_to_copy_across_objects: "
         "(%zu)\n",
         total_objects_to_copy, total_blocks_to_copy_all_objects);
}

// Copy source part object to destination
void S3PutMultipartCopyAction::copy_part_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (!total_data_to_copy) {
    s3_log(S3_LOG_DEBUG, stripped_request_id,
           "Source object/specified range is empty");
    next();
    return;
  }
  bool f_success = false;
  try {
    object_data_copier.reset(new S3ObjectDataCopier(
        request, motr_writer, motr_reader_factory, s3_motr_api));

    object_data_copier->copy(
        additional_object_metadata->get_oid(), total_data_to_copy,
        additional_object_metadata->get_layout_id(),
        additional_object_metadata->get_pvid(),
        std::bind(&S3PutMultipartCopyAction::copy_part_object_cb, this),
        std::bind(&S3PutMultipartCopyAction::copy_part_object_success, this),
        std::bind(&S3PutMultipartCopyAction::copy_part_object_failed, this),
        first_byte_offset_to_copy);
    f_success = true;
  }
  catch (const std::exception& ex) {
    s3_log(S3_LOG_ERROR, stripped_request_id, "%s", ex.what());
  }
  catch (...) {
    s3_log(S3_LOG_ERROR, stripped_request_id, "Non-standard C++ exception");
  }
  if (!f_success) {
    object_data_copier.reset();

    set_s3_error("InternalError");
    send_response_to_s3_client();
    return;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::copy_part_object_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_copy_part_action_state = S3PutObjectActionState::writeComplete;
  object_data_copier.reset();
  next();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::copy_part_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_copy_part_action_state = S3PutObjectActionState::writeFailed;
  set_s3_error(object_data_copier->get_s3_error());

  object_data_copier.reset();
  send_response_to_s3_client();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::save_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Reset 'Date' and 'Last-Modified' time object metadata
  part_metadata->reset_date_time_to_current();
  part_metadata->set_content_length(std::to_string(total_data_to_copy));
  part_metadata->set_md5(motr_writer->get_content_md5());
  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      part_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);

  if (s3_fi_is_enabled("fail_save_part_mdata")) {
    s3_fi_enable_once("motr_kv_put_fail");
  }

  part_metadata->save(
      std::bind(&S3PutMultipartCopyAction::save_object_metadata_success, this),
      std::bind(&S3PutMultipartCopyAction::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_copy_part_action_state = S3PutObjectActionState::metadataSaved;
  next();
}

void S3PutMultipartCopyAction::save_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_copy_part_action_state = S3PutObjectActionState::metadataSaveFailed;
  if (part_metadata->get_state() == S3PartMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Save of Part metadata failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Save of Part metadata failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

std::string S3PutMultipartCopyAction::get_response_xml() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  std::string xml_response = "";
  xml_response += "<CopyPartResult>\n";
  xml_response += S3CommonUtilities::format_xml_string(
      "ETag", motr_writer->get_content_md5(), true);
  xml_response += S3CommonUtilities::format_xml_string(
      "LastModified", additional_object_metadata->get_last_modified_iso());
  xml_response += "\n</CopyPartResult>";
  s3_log(S3_LOG_INFO, stripped_request_id, "Base xml: %s",
         xml_response.c_str());

  return xml_response;
}

bool S3PutMultipartCopyAction::if_source_and_destination_same() {
  return ((additional_bucket_name == request->get_bucket_name()) &&
          (additional_object_name == request->get_object_name()));
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi || old_object_oid.u_lo) {
    assert(!old_oid_str.empty());

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(
        old_oid_str, part_metadata->get_content_length());

    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding old_probable_del_rec with key [%s]\n",
           old_oid_rec_key.c_str());
    old_probable_del_rec.reset(new S3ProbableDeleteRecord(
        old_oid_rec_key, {0ULL, 0ULL},
        object_multipart_metadata->get_object_name(), old_object_oid,
        old_layout_id, object_multipart_metadata->get_pvid_str(),
        bucket_metadata->get_multipart_index_layout().oid,
        bucket_metadata->get_objects_version_list_index_layout().oid, "",
        false /* force_delete */, true,
        part_metadata->get_part_index_layout().oid, 0,
        strtoul(part_metadata->get_part_number().c_str(), NULL, 0),
        bucket_metadata->get_extended_metadata_index_layout().oid));

    probable_oid_list[old_oid_rec_key] = old_probable_del_rec->to_json();
  }
  // prepending a char depending on the size of the object (size based bucketing
  // of object)
  S3CommonUtilities::size_based_bucketing_of_objects(
      new_oid_str, request->get_content_length());

  s3_log(S3_LOG_DEBUG, request_id,
         "Adding new_probable_del_rec with key [%s]\n", new_oid_str.c_str());
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      new_oid_str, old_object_oid, object_multipart_metadata->get_object_name(),
      new_object_oid, layout_id, object_multipart_metadata->get_pvid_str(),
      bucket_metadata->get_multipart_index_layout().oid,
      bucket_metadata->get_objects_version_list_index_layout().oid, "",
      false /* force_delete */, true,
      part_metadata->get_part_index_layout().oid, 1,
      strtoul(part_metadata->get_part_number().c_str(), NULL, 0),
      bucket_metadata->get_extended_metadata_index_layout().oid));
  // store new oid, key = newoid
  probable_oid_list[new_oid_str] = new_probable_del_rec->to_json();

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_layout, probable_oid_list,
      std::bind(&S3PutMultipartCopyAction::next, this),
      std::bind(&S3PutMultipartCopyAction::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void
S3PutMultipartCopyAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_copy_part_action_state = S3PutObjectActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (S3Option::get_instance()->is_getoid_enabled()) {

    request->set_out_header_value("x-stx-oid",
                                  S3M0Uint128Helper::to_string(new_object_oid));
    request->set_out_header_value("x-stx-layout-id", std::to_string(layout_id));
  }
  std::string response_xml;
  int http_status_code = S3HttpFailed500;

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    // Metadata saved for object is always a success condition.
    assert(s3_copy_part_action_state != S3PutObjectActionState::metadataSaved);

    S3Error error(get_s3_error_code(), request->get_request_id());

    if (S3PutObjectActionState::validationFailed == s3_copy_part_action_state &&
        "InvalidRequest" == get_s3_error_code()) {
      if (if_source_and_destination_same()) {  // Source and Destination same
        error.set_auth_error_message(
            InvalidRequestPartCopySourceAndDestinationSame);
      } else if (source_object_size >
                 MaxPartCopySourcePartSize) {  // Source object size greater
                                               // than 5GB
        error.set_auth_error_message(
            InvalidRequestSourcePartSizeGreaterThan5GB);
      }
    }
    response_xml = std::move(error.to_xml(response_started));
    http_status_code = error.get_http_status_code();

    if (!response_started && (get_s3_error_code() == "ServiceUnavailable" ||
                              get_s3_error_code() == "InternalError")) {
      request->set_out_header_value("Connection", "close");
    }
    if (get_s3_error_code() == "ServiceUnavailable") {
      if (reject_if_shutting_down()) {
        int retry_after_period =
            S3Option::get_instance()->get_s3_retry_after_sec();
        request->set_out_header_value("Retry-After",
                                      std::to_string(retry_after_period));
      } else {
        request->set_out_header_value("Retry-After", "1");
      }
    }
  } else {
    s3_copy_part_action_state = S3PutObjectActionState::completed;

    response_xml = get_response_xml();
    http_status_code = S3HttpSuccess200;
  }
  if (response_started) {
    request->send_reply_body(response_xml.c_str(), response_xml.length());
    request->send_reply_end();
    request->close_connection();
  } else {
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(http_status_code, std::move(response_xml));
  }
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::set_source_bucket_authorization_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  auth_acl = request->get_default_acl();
  auth_client->set_get_method = true;

  request->reset_action_list();
  auth_client->set_entity_path("/" + additional_bucket_name + "/" +
                               additional_object_name);

  auth_client->set_acl_and_policy(
      additional_object_metadata->get_encoded_object_acl(),
      additional_bucket_metadata->get_policy_as_json());
  request->set_action_str("GetObject");
  request->reset_action_list();
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::check_source_bucket_authorization() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  auth_client->check_authorization(
      std::bind(
          &S3PutMultipartCopyAction::check_source_bucket_authorization_success,
          this),
      std::bind(
          &S3PutMultipartCopyAction::check_source_bucket_authorization_failed,
          this));
}

void S3PutMultipartCopyAction::check_source_bucket_authorization_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::check_source_bucket_authorization_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_copy_part_action_state = S3PutObjectActionState::validationFailed;
  std::string error_code = auth_client->get_error_code();

  set_s3_error(error_code);
  s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
         error_code.c_str());

  if (request->client_connected()) {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::startcleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // TODO: Perf - all/some of below tasks can be done in parallel
  // Any of the following steps fail, backgrounddelete will be able to perform
  // cleanups.
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;
  // Success conditions
  if (s3_copy_part_action_state == S3PutObjectActionState::completed ||
      s3_copy_part_action_state == S3PutObjectActionState::metadataSaved) {
    s3_log(S3_LOG_DEBUG, request_id, "Cleanup old Object\n");
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // mark old OID for deletion in overwrite case, this optimizes
      // backgrounddelete decisions.
      ACTION_TASK_ADD(S3PutMultipartCopyAction::mark_old_oid_for_deletion,
                      this);
    }
    // remove new oid from probable delete list.
    ACTION_TASK_ADD(S3PutMultipartCopyAction::remove_new_oid_probable_record,
                    this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // Object overwrite case, old object exists, delete it.
      ACTION_TASK_ADD(S3PutMultipartCopyAction::delete_old_object, this);
      // If delete object is successful, attempt to delete old probable record
    }
  } else if (s3_copy_part_action_state ==
                 S3PutObjectActionState::newObjOidCreated ||
             s3_copy_part_action_state == S3PutObjectActionState::writeFailed ||
             s3_copy_part_action_state ==
                 S3PutObjectActionState::md5ValidationFailed ||
             s3_copy_part_action_state ==
                 S3PutObjectActionState::metadataSaveFailed) {
    // PUT is assumed to be failed with a need to rollback
    s3_log(S3_LOG_DEBUG, request_id,
           "Cleanup new Object: s3_copy_part_action_state[%d]\n",
           s3_copy_part_action_state);
    // Mark new OID for deletion, this optimizes backgrounddelete decisionss.
    ACTION_TASK_ADD(S3PutMultipartCopyAction::mark_new_oid_for_deletion, this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // remove old oid from probable delete list.
      ACTION_TASK_ADD(S3PutMultipartCopyAction::remove_old_oid_probable_record,
                      this);
    }
    ACTION_TASK_ADD(S3PutMultipartCopyAction::delete_new_object, this);
    // If delete object is successful, attempt to delete new probable record
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "No Cleanup required: s3_copy_part_action_state[%d]\n",
           s3_copy_part_action_state);
    assert(s3_copy_part_action_state == S3PutObjectActionState::empty ||
           s3_copy_part_action_state ==
               S3PutObjectActionState::validationFailed ||
           s3_copy_part_action_state ==
               S3PutObjectActionState::probableEntryRecordFailed ||
           s3_copy_part_action_state ==
               S3PutObjectActionState::newObjOidCreationFailed);
    // Nothing to undo
  }
  // Start running the cleanup task list
  start();
}
void S3PutMultipartCopyAction::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  // update new oid, key = newoid, force_del = true
  new_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             new_oid_str, new_probable_del_rec->to_json(),
                             std::bind(&S3PutMultipartCopyAction::next, this),
                             std::bind(&S3PutMultipartCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  std::string prepended_new_oid_str = new_oid_str;
  // key = oldoid + "-" + newoid

  std::string old_oid_rec_key =
      old_oid_str + '-' + prepended_new_oid_str.erase(0, 1);

  // update old oid, force_del = true
  old_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             old_oid_rec_key, old_probable_del_rec->to_json(),
                             std::bind(&S3PutMultipartCopyAction::next, this),
                             std::bind(&S3PutMultipartCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      global_probable_dead_object_list_index_layout, old_oid_rec_key,
      std::bind(&S3PutMultipartCopyAction::next, this),
      std::bind(&S3PutMultipartCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutMultipartCopyAction::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!new_oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      global_probable_dead_object_list_index_layout, new_oid_str,
      std::bind(&S3PutMultipartCopyAction::next, this),
      std::bind(&S3PutMultipartCopyAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::delete_old_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT is success, we delete old object if present
  assert(old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL);

  // If old part exists and deletion of old is disabled, then return
  if ((old_object_oid.u_hi || old_object_oid.u_lo) &&
      S3Option::get_instance()->is_s3server_obj_delayed_del_enabled()) {
    s3_log(S3_LOG_INFO, request_id,
           "Skipping deletion of old object. The old object will be deleted by "
           "BD.\n");
    // Call next task in the pipeline
    next();
    return;
  }
  motr_writer->set_oid(old_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutMultipartCopyAction::remove_old_oid_probable_record,
                this),
      std::bind(&S3PutMultipartCopyAction::next, this), old_object_oid,
      old_layout_id, object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// TODO: Add this function to a common class
void S3PutMultipartCopyAction::delete_new_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // If PUT failed, then clean new object.
  assert(s3_copy_part_action_state != S3PutObjectActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);

  motr_writer->set_oid(new_object_oid);
  motr_writer->delete_object(
      std::bind(&S3PutMultipartCopyAction::remove_new_oid_probable_record,
                this),
      std::bind(&S3PutMultipartCopyAction::next, this), new_object_oid,
      layout_id, object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

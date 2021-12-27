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

#include <algorithm>
#include "s3_get_object_action.h"
#include "s3_motr_layout.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_common_utilities.h"
#include "s3_stats.h"
#include "s3_perf_metrics.h"
#include "s3_m0_uint128_helper.h"

S3GetObjectAction::S3GetObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrReaderFactory> motr_s3_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory)),
      total_blocks_in_object(0),
      blocks_already_read(0),
      data_sent_to_client(0),
      content_length(0),
      first_byte_offset_to_read(0),
      last_byte_offset_to_read(0),
      total_blocks_to_read(0),
      read_object_reply_started(false),
      total_objects(0),
      total_objects_to_read(0),
      next_fragment_object(0),
      total_blocks_to_read_all_objects(0),
      data_sent_to_client_for_object(0) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Get Object. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  if (motr_s3_factory) {
    motr_reader_factory = std::move(motr_s3_factory);
  } else {
    motr_reader_factory = std::make_shared<S3MotrReaderFactory>();
  }

  setup_steps();
}

void S3GetObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3GetObjectAction::validate_object_info, this);
  ACTION_TASK_ADD(S3GetObjectAction::check_full_or_range_object_read, this);
  ACTION_TASK_ADD(S3GetObjectAction::read_object, this);
  ACTION_TASK_ADD(S3GetObjectAction::send_response_to_s3_client, this);
  // ...
}

void S3GetObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::fetch_object_info_failed() {
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Found bucket metadata\n");
    obj_list_idx_lo = bucket_metadata->get_object_list_index_layout();
    if (zero(obj_list_idx_lo.oid)) {
      s3_log(S3_LOG_ERROR, request_id, "Object not found\n");
      set_s3_error("NoSuchKey");
    } else {
      if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
        s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
        set_s3_error("NoSuchKey");
      } else if (object_metadata->get_state() ==
                 S3ObjectMetadataState::failed_to_launch) {
        s3_log(S3_LOG_ERROR, request_id,
               "Object metadata load operation failed due to pre launch "
               "failure\n");
        set_s3_error("ServiceUnavailable");
      } else {
        s3_log(S3_LOG_DEBUG, request_id, "Object metadata fetch failed\n");
        set_s3_error("InternalError");
      }
    }
  }
  send_response_to_s3_client();
}

void S3GetObjectAction::validate_object_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  content_length = object_metadata->get_content_length();
  request->set_object_size(content_length);
  // as per RFC last_byte_offset_to_read is taken to be equal to one less than
  // the content length in bytes.
  last_byte_offset_to_read =
      (content_length == 0) ? content_length : (content_length - 1);
  s3_log(S3_LOG_DEBUG, request_id, "Found object of size %zu\n",
         content_length);
  if (object_metadata->check_object_tags_exists()) {
    request->set_out_header_value(
        "x-amz-tagging-count",
        std::to_string(object_metadata->object_tags_count()));
  }

  if (content_length == 0) {
    // AWS add explicit quotes "" to etag values.
    // https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html
    std::string e_tag = "\"" + object_metadata->get_md5() + "\"";

    request->set_out_header_value("Last-Modified",
                                  object_metadata->get_last_modified_gmt());
    request->set_out_header_value("ETag", e_tag);
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Type",
                                  object_metadata->get_content_type());
    request->set_out_header_value("Content-Length",
                                  object_metadata->get_content_length_str());
    for (auto it : object_metadata->get_user_attributes()) {
      request->set_out_header_value(it.first, it.second);
    }
    request->send_reply_start(S3HttpSuccess200);
    send_response_to_s3_client();
  } else {
    if (this->object_metadata->get_number_of_fragments() == 0) {
      // Object is normal (not fragmented)
      total_objects = 1;
      size_t motr_unit_size =
          S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
              object_metadata->get_layout_id());
      s3_log(S3_LOG_DEBUG, request_id,
             "motr_unit_size = %zu for layout_id = %d\n", motr_unit_size,
             object_metadata->get_layout_id());
      /* Count Data blocks from data size */
      total_blocks_in_object =
          (content_length + (motr_unit_size - 1)) / motr_unit_size;
      s3_log(S3_LOG_DEBUG, request_id, "total_blocks_in_object: (%zu)\n",
             total_blocks_in_object);
    } else {
      bool is_multipart = false;
      // Object is fragmented and/or parts
      unsigned int ext_entry_index = 0;
      if (object_metadata->get_number_of_parts() == 0) {
        // Object is only fragmented then:= total objects = fragments + 1
        total_objects = object_metadata->get_number_of_fragments() + 1;
      } else {
        // Object is multipart (fragmented or not) then total object = fragments
        // When object is multipart, object_metadata->get_number_of_parts() > 0
        total_objects = object_metadata->get_number_of_fragments();
        is_multipart = true;
      }
      s3_log(S3_LOG_DEBUG, request_id, "Total objects : (%zu)\n",
             total_objects);
      const std::shared_ptr<S3ObjectExtendedMetadata>& ext_obj =
          object_metadata->get_extended_object_metadata();
      const std::map<int, std::vector<struct s3_part_frag_context>>&
          ext_entries = ext_obj->get_raw_extended_entries();
      for (unsigned int i = 0; i < total_objects; i++) {
        struct S3ExtendedObjectInfo obj_info;
        if (!is_multipart) {
          // non-multipart fragmented object
          if (i == 0) {
            // Primary object
            obj_info.start_offset_in_object = 0;
            obj_info.object_OID = object_metadata->get_oid();
            obj_info.object_layout = object_metadata->get_layout_id();
            obj_info.object_pvid = object_metadata->get_pvid();
            obj_info.object_size = object_metadata->get_primary_obj_size();
            obj_info.requested_object_size = obj_info.object_size;
            size_t motr_unit_size =
                S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
                    object_metadata->get_layout_id());
            /* Count Data blocks from data size */
            obj_info.total_blocks_in_object =
                (obj_info.object_size + (motr_unit_size - 1)) / motr_unit_size;
            total_blocks_in_object += obj_info.total_blocks_in_object;
            s3_log(S3_LOG_DEBUG, request_id,
                   "motr_unit_size = %zu for layout_id = %d in object (oid) ="
                   "%" SCNx64 " : %" SCNx64
                   " with blocks in object = (%zu),"
                   " starting object offset = (%zu)\n",
                   motr_unit_size, object_metadata->get_layout_id(),
                   obj_info.object_OID.u_hi, obj_info.object_OID.u_lo,
                   obj_info.total_blocks_in_object,
                   obj_info.start_offset_in_object);
            extended_objects.push_back(obj_info);
          } else {
            obj_info.start_offset_in_object =
                extended_objects[i - 1].start_offset_in_object +
                extended_objects[i - 1].object_size;
            const struct s3_part_frag_context& frag_info =
                (ext_entries.at(0)).at(ext_entry_index);
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
                   "%" SCNx64 " : %" SCNx64
                   " with blocks in object = (%zu),"
                   " starting object offset = (%zu)\n",
                   motr_unit_size, obj_info.object_layout,
                   obj_info.object_OID.u_hi, obj_info.object_OID.u_lo,
                   obj_info.total_blocks_in_object,
                   obj_info.start_offset_in_object);
            extended_objects.push_back(obj_info);
            ++ext_entry_index;
          }
        } else {
          // multipart (may be fragmented) object
          // For now, ext_entry_index is always 0,
          // assuming multipart is not fragmented.
          ext_entry_index = 0;
          if (i == 0) {
            // first part object
            obj_info.start_offset_in_object = 0;
          } else {
            obj_info.start_offset_in_object =
                extended_objects[i - 1].start_offset_in_object +
                extended_objects[i - 1].object_size;
          }
          // For multipart, ext_entries are indexed from key=1 onwards
          const struct s3_part_frag_context& frag_info =
              (ext_entries.at(i + 1)).at(ext_entry_index);
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
                 "%" SCNx64 " : %" SCNx64
                 " with blocks in object = (%zu),"
                 " starting object offset = (%zu)\n",
                 motr_unit_size, obj_info.object_layout,
                 obj_info.object_OID.u_hi, obj_info.object_OID.u_lo,
                 obj_info.total_blocks_in_object,
                 obj_info.start_offset_in_object);
          extended_objects.push_back(obj_info);
        }
      }  // End of For loop
      s3_log(S3_LOG_DEBUG, request_id,
             "total_blocks_in_all_fragmented_object: (%zu)\n",
             total_blocks_in_object);
    }
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::set_total_blocks_to_read_from_object() {
  // to read complete object, total number blocks to read is equal to total
  // number of blocks
  if ((first_byte_offset_to_read == 0) &&
      (last_byte_offset_to_read == (content_length - 1))) {
    total_blocks_to_read = total_blocks_in_object;
  } else {
    // object read for valid range
    // get total number blocks to read for a given valid range
    size_t motr_unit_size =
        S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
            object_metadata->get_layout_id());
    // get block of first_byte_offset_to_read
    size_t first_byte_offset_block =
        (first_byte_offset_to_read + motr_unit_size) / motr_unit_size;
    // get block of last_byte_offset_to_read
    size_t last_byte_offset_block =
        (last_byte_offset_to_read + motr_unit_size) / motr_unit_size;
    // get total number blocks to read for a given valid range
    total_blocks_to_read = last_byte_offset_block - first_byte_offset_block + 1;
  }
}

void S3GetObjectAction::set_total_blocks_to_read_from_fragmented_object() {
  if ((first_byte_offset_to_read == 0) &&
      (last_byte_offset_to_read == (content_length - 1))) {
    // Number of blocks to read from all objects, starting from primary
    // object to last extended object.
    for (unsigned int i = 0; i < total_objects; i++) {
      total_blocks_to_read_all_objects +=
          extended_objects[i].total_blocks_in_object;
      extended_objects[i].total_readable_blocks =
          extended_objects[i].total_blocks_in_object;
    }
    // In order to read complete object, total number of objects to read
    // is equal to total number of objects
    total_objects_to_read = total_objects;
  } else {
    // TODO: Correct the implementtaion below for fragmented object,
    // when byte range is specified.
    // object read for valid range
    // TODO: Find the object (i.e index in extended_objects) containing
    // first_byte_offset_to_read
    unsigned int first_byte_object_index = 0,
                 last_byte_object_index = total_objects - 1;
    total_objects_to_read = total_objects;
    for (unsigned int i = 0; i < total_objects; i++) {
      if (first_byte_offset_to_read <
          (extended_objects[i].start_offset_in_object +
           extended_objects[i].object_size)) {
        // Found first_byte_offset_to_read in object at index i
        first_byte_object_index = i;
        break;
      }
    }
    // TODO: Find the object containing last_byte_offset_to_read
    for (unsigned int j = first_byte_object_index; j < total_objects; j++) {
      if (last_byte_offset_to_read <
          (extended_objects[j].start_offset_in_object +
           extended_objects[j].object_size)) {
        // Found last_byte_offset_to_read in object at index i
        last_byte_object_index = j;
        break;
      }
    }
    // Calculate the actual requested content in first object
    // (first_byte_object_index) and last object (last_byte_object_index)
    if (last_byte_object_index == first_byte_object_index) {
      // If both first byte offset and last byte offset are in same object
      extended_objects[first_byte_object_index].requested_object_size =
          last_byte_offset_to_read - first_byte_offset_to_read + 1;
    } else {
      extended_objects[first_byte_object_index].requested_object_size =
          (extended_objects[first_byte_object_index].object_size -
           (first_byte_offset_to_read -
            extended_objects[first_byte_object_index].start_offset_in_object) +
           1);

      extended_objects[last_byte_object_index].requested_object_size =
          last_byte_offset_to_read -
          (extended_objects[last_byte_object_index].start_offset_in_object) + 1;
    }

    // Get the block of first_byte_offset_to_read from object at
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
        (first_byte_offset_to_read -
         extended_objects[first_byte_object_index].start_offset_in_object +
         unit_size_of_object_with_first_byte) /
        unit_size_of_object_with_first_byte;

    // Get the block of last_byte_offset_to_read from object at
    // index 'last_byte_object_index'
    size_t last_byte_offset_block =
        (last_byte_offset_to_read -
         extended_objects[last_byte_object_index].start_offset_in_object +
         unit_size_of_object_with_last_byte) /
        unit_size_of_object_with_last_byte;

    // Get total number blocks to read for a given valid range
    for (unsigned int k = first_byte_object_index; k <= last_byte_object_index;
         k++) {
      // Calculate blocks in each object in the valid object range with offsets
      if (k == first_byte_object_index) {
        extended_objects[k].total_readable_blocks =
            extended_objects[k].total_blocks_in_object -
            first_byte_offset_block + 1;
        total_blocks_to_read_all_objects +=
            extended_objects[k].total_readable_blocks;
      } else if (k == last_byte_object_index) {
        extended_objects[k].total_readable_blocks = last_byte_offset_block;
        total_blocks_to_read_all_objects += last_byte_offset_block;
      } else {
        total_blocks_to_read_all_objects +=
            extended_objects[k].total_blocks_in_object;
        extended_objects[k].total_readable_blocks =
            extended_objects[k].total_blocks_in_object;
      }
    }
    // Check if 'extended_objects' needs to be shrunk, due to specified byte
    // range
    bool need_to_shrink_front = false, need_to_shrink_end = false;
    // First, check for first byte offset object
    if (first_byte_object_index != 0 &&
        first_byte_object_index < total_objects) {
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
    if (need_to_shrink_front || need_to_shrink_end) {
      if (need_to_shrink_front && need_to_shrink_end) {
        std::copy(extended_objects.begin() + first_byte_object_index,
                  (extended_objects.begin() + last_byte_object_index + 1),
                  back_inserter(new_set_of_extended_objects));
      } else if (need_to_shrink_front) {
        // Shrink front only
        std::copy(extended_objects.begin() + first_byte_object_index,
                  extended_objects.end(),
                  back_inserter(new_set_of_extended_objects));
      } else {
        // Shrink end only
        std::copy(extended_objects.begin(),
                  (extended_objects.begin() + last_byte_object_index + 1),
                  back_inserter(new_set_of_extended_objects));
      }
      // Re-set the array of objects to read data from
      extended_objects = new_set_of_extended_objects;
      // Re-calculate total objects to read data from
      total_objects_to_read = extended_objects.size();
    }
  }
  s3_log(S3_LOG_DEBUG, request_id,
         "total_objects_to_read: (%zu)\n,total_blocks_to_read_across_objects: "
         "(%zu)\n",
         total_objects_to_read, total_blocks_to_read_all_objects);
}

bool S3GetObjectAction::validate_range_header_and_set_read_options(
    const std::string& range_value) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // The header can consist of 'blank' character(s) only
  if (std::find_if_not(range_value.begin(), range_value.end(), &::isspace) ==
      range_value.end()) {
    s3_log(S3_LOG_DEBUG, request_id,
           "\"Range:\" header consists of blank symbol(s) only");
    return true;
  }
  // reference: http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35.
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
    // in this case, AWS returns full object and hence we do too
    s3_log(S3_LOG_INFO, stripped_request_id, "unsupported multirange(%s)\n",
           byte_range_set.c_str());
    // initialize the first and last offset values with actual object offsets
    // to read complte object
    first_byte_offset_to_read = 0;
    last_byte_offset_to_read = content_length - 1;
    return true;
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
  // -nnn
  // Return last 'nnn' bytes from object.
  if (first_byte.empty()) {
    first_byte_offset_to_read = content_length - atol(last_byte.c_str());
    last_byte_offset_to_read = content_length - 1;
  } else if (last_byte.empty()) {
    // nnn-
    // Return from 'nnn' bytes to content_length-1 from object.
    first_byte_offset_to_read = atol(first_byte.c_str());
    last_byte_offset_to_read = content_length - 1;
  } else {
    // both are not empty
    first_byte_offset_to_read = atol(first_byte.c_str());
    last_byte_offset_to_read = atol(last_byte.c_str());
  }
  // last_byte_offset_to_read is greater than or equal to the current length of
  // the entity-body, last_byte_offset_to_read is taken to be equal to
  // one less than the current length of the entity- body in bytes.
  if (last_byte_offset_to_read > content_length - 1) {
    last_byte_offset_to_read = content_length - 1;
  }
  // Range validation
  // If a syntactically valid byte-range-set includes at least one byte-
  // range-spec whose first-byte-pos is less than the current length of the
  // entity-body, or at least one suffix-byte-range-spec with a non- zero
  // suffix-length, then the byte-range-set is satisfiable.
  if ((first_byte_offset_to_read >= content_length) ||
      (first_byte_offset_to_read > last_byte_offset_to_read)) {
    s3_log(S3_LOG_INFO, stripped_request_id, "Invalid range(%s)\n",
           range_value.c_str());
    return false;
  }
  // valid range
  s3_log(S3_LOG_DEBUG, request_id, "valid range(%zu-%zu) found\n",
         first_byte_offset_to_read, last_byte_offset_to_read);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return true;
}

void S3GetObjectAction::check_full_or_range_object_read() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string range_header_value = request->get_header_value("Range");
  if (range_header_value.empty()) {
    // Range is not specified, read complte object
    s3_log(S3_LOG_DEBUG, request_id, "Range is not specified\n");
    next();
  } else {
    // parse the Range header value
    // eg: bytes=0-1024 value
    s3_log(S3_LOG_DEBUG, request_id, "Range found(%s)\n",
           range_header_value.c_str());
    if (validate_range_header_and_set_read_options(range_header_value)) {
      next();
    } else {
      set_s3_error("InvalidRange");
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::read_fragmented_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  set_total_blocks_to_read_from_next_object();
  motr_reader = motr_reader_factory->create_motr_reader(
      request, extended_objects[next_fragment_object].object_OID,
      extended_objects[next_fragment_object].object_layout,
      extended_objects[next_fragment_object].object_pvid);
  // get the block,in which first_byte_offset_to_read is present
  // and initilaize the last index with starting offset of the block
  size_t block_start_offset = 0;
  if (next_fragment_object == 0) {
    size_t unit_size_of_object_with_first_byte =
        S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
            extended_objects[next_fragment_object].object_layout);

    size_t first_byte_offset_block =
        (first_byte_offset_to_read -
         extended_objects[next_fragment_object].start_offset_in_object +
         unit_size_of_object_with_first_byte) /
        unit_size_of_object_with_first_byte;

    block_start_offset =
        (first_byte_offset_block - 1) * unit_size_of_object_with_first_byte;
  }
  motr_reader->set_last_index(block_start_offset);
  read_object_data();
  s3_log(S3_LOG_INFO, "", "%s Exit", __func__);
}

void S3GetObjectAction::set_total_blocks_to_read_from_next_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  blocks_already_read = 0;
  data_sent_to_client_for_object = 0;
  total_blocks_to_read =
      extended_objects[next_fragment_object].total_readable_blocks;

  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit\n", __func__);
}

void S3GetObjectAction::read_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (this->object_metadata->get_number_of_fragments() == 0) {
    // Object is not fragmented
    // get total number of blocks to read from an object
    set_total_blocks_to_read_from_object();
    motr_reader = motr_reader_factory->create_motr_reader(
        request, object_metadata->get_oid(), object_metadata->get_layout_id(),
        object_metadata->get_pvid());
    // get the block,in which first_byte_offset_to_read is present
    // and initilaize the last index with starting offset the block
    size_t block_start_offset =
        first_byte_offset_to_read -
        (first_byte_offset_to_read %
         S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
             object_metadata->get_layout_id()));
    motr_reader->set_last_index(block_start_offset);
    read_object_data();
  } else {
    set_total_blocks_to_read_from_fragmented_object();
    read_fragmented_object();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::check_outbuffer_and_mempool_stats(bool& bcontinue) {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  bcontinue = true;
  size_t motr_read_payload_size =
      S3Option::get_instance()->get_motr_read_payload_size(
          object_metadata->get_layout_id());
  // Determine the size of outstanding response buffer (not written to client
  // sock) in Lib event
  size_t len_response_buffer = request->get_write_buffer_outstanding_length();
  size_t len_mempool_free_mem = 0;
  // Determine the size of outstanding mem pool buffer (free buffer)
  struct pool_info poolinfo;
  int rc = event_mempool_getinfo(&poolinfo);
  if (rc != 0) {
    s3_log(
        S3_LOG_ERROR, request_id,
        "Issue in reading memory pool stats during S3 Get API memory check\n");
  } else {
    len_mempool_free_mem =
        poolinfo.free_bufs_in_pool * poolinfo.mempool_item_size;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Outstanding S3 Get response buffer size: (%zu)\n",
         len_response_buffer);
  s3_log(S3_LOG_INFO, request_id, "Free S3 mempool memory: (%zu)\n",
         len_mempool_free_mem);
  if ((len_response_buffer >=
       (motr_read_payload_size *
        S3Option::get_instance()->get_write_buffer_multiple())) ||
      (!S3MemoryProfile().free_memory_in_pool_above_threshold_limits())) {
    bcontinue = false;
    s3_log(
        S3_LOG_WARN, stripped_request_id,
        "Limited memory: Throttling S3 GET object/part request is required\n");
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::read_object_data() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  // Before reading from Motr, ensure that outstanding response buffer
  // is not above threshold1 or outstanding S3 mempool memory
  // is not below threshold2
  // threshold1 := motr_read_payload_size *
  // S3Option::get_instance()->get_write_buffer_multiple()
  // threshold2 :=
  // S3MemoryProfile().free_memory_in_pool_above_threshold_limits()
  bool bcontinue = true;
  check_outbuffer_and_mempool_stats(bcontinue);
  if (!bcontinue) {
    int throttle_for_millisecs =
        S3Option::get_instance()->get_s3_req_throttle_time();
    // Throttle S3 Get API by adding delay using timer event
    if (!request->set_start_response_delay_timer(throttle_for_millisecs,
                                                 (void*)this)) {
      s3_log(S3_LOG_INFO, request_id,
             "S3 GET API response will be throttled by: (%d) millisecs\n",
             throttle_for_millisecs);
      return;
    } else {
      s3_log(S3_LOG_WARN, request_id,
             "Failed to throttle S3 GET API response\n");
    }
  }
  size_t max_blocks_in_one_read_op =
      S3Option::get_instance()->get_motr_units_per_request();
  size_t motr_unit_size = 0;
  if (!object_metadata->is_object_extended()) {
    motr_unit_size = S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
        object_metadata->get_layout_id());
  } else {
    motr_unit_size = S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
        extended_objects[next_fragment_object].object_layout);
  }
  blocks_to_read = 0;

  s3_log(S3_LOG_DEBUG, request_id, "max_blocks_in_one_read_op: (%zu)\n",
         max_blocks_in_one_read_op);
  s3_log(S3_LOG_DEBUG, request_id, "blocks_already_read: (%zu)\n",
         blocks_already_read);
  s3_log(S3_LOG_DEBUG, request_id, "total_blocks_to_read: (%zu)\n",
         total_blocks_to_read);
  if (blocks_already_read != total_blocks_to_read) {
    if (blocks_already_read == 0 && next_fragment_object == 0 &&
        get_requested_content_length() >
            max_blocks_in_one_read_op * motr_unit_size) {
      size_t first_blocks_to_read =
          S3Option::get_instance()->get_motr_first_read_size();
      blocks_to_read = max_blocks_in_one_read_op < first_blocks_to_read
                           ? max_blocks_in_one_read_op
                           : first_blocks_to_read;
      s3_log(S3_LOG_DEBUG, request_id, "First blocks_to_read: (%zu)\n",
             blocks_to_read);
    } else if ((total_blocks_to_read - blocks_already_read) >
               max_blocks_in_one_read_op) {
      blocks_to_read = max_blocks_in_one_read_op;
    } else {
      blocks_to_read = total_blocks_to_read - blocks_already_read;
    }
    // Below is only for first block read op and non-zero block start offset.
    // For the first block read op, if reading starts from non-zero offset
    // adjust number of 'blocks_to_read', so that the read does not go
    // beyond the object size.
    if (blocks_already_read == 0 && (motr_reader->get_last_index() != 0)) {
      /*
        size_t unit_size_of_object_with_first_byte =
          S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
              extended_objects[next_fragment_object].object_layout);
      // Number of initial blocks skipped
      size_t blocks_skipped =
          motr_reader->get_last_index() / unit_size_of_object_with_first_byte;
      // Make sure blocks_to_read is not going beyond the object size
      size_t effective_blocks_in_object =
          extended_objects[next_fragment_object].total_blocks_in_object -
          blocks_skipped;
      */
      if (blocks_to_read >
          extended_objects[next_fragment_object].total_readable_blocks) {
        blocks_to_read =
            extended_objects[next_fragment_object].total_readable_blocks;
      }
    }

    s3_log(S3_LOG_DEBUG, request_id, "blocks_to_read: (%zu)\n", blocks_to_read);

    if (blocks_to_read > 0) {
      bool op_launched = motr_reader->read_object_data(
          blocks_to_read,
          std::bind(&S3GetObjectAction::send_data_to_client, this),
          std::bind(&S3GetObjectAction::read_object_data_failed, this));
      if (!op_launched) {
        if (motr_reader->get_state() == S3MotrReaderOpState::failed_to_launch) {
          set_s3_error("ServiceUnavailable");
          s3_log(S3_LOG_ERROR, request_id,
                 "read_object_data called due to motr_entity_open failure\n");
        } else {
          set_s3_error("InternalError");
        }
        send_response_to_s3_client();
      }
    } else {
      send_response_to_s3_client();
    }
  } else {
    // We are done Reading
    // Check if this is an extended object. If not, send response to client.
    // If extended object, open next fragmented object, and continue calling
    // read_fragmented_object()
    if (this->object_metadata->get_number_of_fragments() != 0) {
      // Read next fragmented object
      next_fragment_object++;
      if (next_fragment_object < total_objects_to_read) {
        read_fragmented_object();
      } else {
        send_response_to_s3_client();
      }
    } else {
      // This is normal object
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::send_data_to_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_stats_inc("read_object_data_success_count");
  log_timed_counter(get_timed_counter, "outgoing_object_data_blocks");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  if (!read_object_reply_started) {
    s3_timer.start();

    // AWS add explicit quotes "" to etag values.
    // https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html
    std::string e_tag = "\"" + object_metadata->get_md5() + "\"";

    request->set_out_header_value("Last-Modified",
                                  object_metadata->get_last_modified_gmt());
    request->set_out_header_value("Content-Type",
                                  object_metadata->get_content_type());
    request->set_out_header_value("ETag", e_tag);
    s3_log(S3_LOG_INFO, stripped_request_id, "e_tag= %s", e_tag.c_str());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value(
        "Content-Length", std::to_string(get_requested_content_length()));
    for (auto it : object_metadata->get_user_attributes()) {
      request->set_out_header_value(it.first, it.second);
    }
    if (!request->get_header_value("Range").empty()) {
      std::ostringstream content_range_stream;
      content_range_stream << "bytes " << first_byte_offset_to_read << "-"
                           << last_byte_offset_to_read << "/" << content_length;
      request->set_out_header_value("Content-Range",
                                    content_range_stream.str());
      // Partial Content
      request->send_reply_start(S3HttpSuccess206);
    } else {
      request->send_reply_start(S3HttpSuccess200);
    }
    read_object_reply_started = true;
  } else {
    s3_timer.resume();
  }
  s3_log(S3_LOG_DEBUG, request_id, "Earlier data_sent_to_client = %zu bytes.\n",
         data_sent_to_client);

  S3Evbuffer* p_evbuffer = motr_reader->get_evbuffer();
  size_t buff_count = (p_evbuffer->get_evbuff_length() + 16384 - 1) / 16384;
  request->add_to_mempool_buffer_count(buff_count);
  size_t obj_unit_sz = 0;
  if (!object_metadata->is_object_extended()) {
    obj_unit_sz = S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
        object_metadata->get_layout_id());
  } else {
    obj_unit_sz = S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
        extended_objects[next_fragment_object].object_layout);
  }
  size_t requested_content_length = get_requested_content_length();
  s3_log(S3_LOG_DEBUG, request_id,
         "object requested content length size(%zu).\n",
         requested_content_length);
  size_t length_in_evbuf = blocks_to_read * obj_unit_sz;
  blocks_already_read += blocks_to_read;
  if (data_sent_to_client == 0) {
    size_t read_data_start_offset = 0;
    // get starting offset from the block,
    // condition true for only statring block read object.
    // this is to set get first offset byte from initial read block
    // eg: read_data_start_offset will be set to 1000 on initial read block
    // for a given range 1000-1500 to read from 2mb object
    if (!object_metadata->is_object_extended()) {
      read_data_start_offset =
          first_byte_offset_to_read %
          S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
              object_metadata->get_layout_id());
    } else {
      // Fragmented object
      read_data_start_offset =
          first_byte_offset_to_read %
          S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
              extended_objects[next_fragment_object].object_layout);
    }
    if (read_data_start_offset) {
      // Move the starting range (1000-) if specified.
      p_evbuffer->drain_data(read_data_start_offset);
      length_in_evbuf = p_evbuffer->get_evbuff_length();
    }
  }
  // to read number of bytes from final read block of read object
  // that is requested content length is lesser than the sum of data has been
  // sent to client and current read block size
  if (((data_sent_to_client + length_in_evbuf) >= requested_content_length) ||
      (p_evbuffer->get_evbuff_length() >= requested_content_length)) {
    // length will have the size of remaining byte to sent
    size_t length = requested_content_length - data_sent_to_client;
    p_evbuffer->read_drain_data_from_buffer(length);
  } else if (object_metadata->is_object_extended() &&
             ((data_sent_to_client_for_object + length_in_evbuf) >=
              extended_objects[next_fragment_object].object_size)) {
    // Remaining bytes in the fragment object
    size_t length = extended_objects[next_fragment_object].object_size -
                    data_sent_to_client_for_object;
    p_evbuffer->read_drain_data_from_buffer(length);
  }
  size_t bytes_sent = p_evbuffer->get_evbuff_length();
  data_sent_to_client += bytes_sent;
  request->set_bytes_sent(data_sent_to_client);
  data_sent_to_client_for_object += bytes_sent;
  // Send data to client. evbuf_body will be free'ed internally
  s3_perf_count_outcoming_bytes(p_evbuffer->get_evbuff_length());
  request->send_reply_body(p_evbuffer->release_ownership());
  s3_log(S3_LOG_DEBUG, request_id, "Sending %zu bytes to client.\n",
         bytes_sent);
  s3_timer.stop();
  // Dump Mem pool stats after sending data to client
  struct pool_info poolinfo;
  int rc = event_mempool_getinfo(&poolinfo);
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Issue in memory pool during S3 Get API send data call!\n");
  } else {
    s3_log(S3_LOG_INFO, request_id,
           "S3 Get API send data mempool stats: mempool_item_size = %zu "
           "free_bufs_in_pool = %d "
           "number_of_bufs_shared = %d "
           "total_bufs_allocated_by_pool = %d\n",
           poolinfo.mempool_item_size, poolinfo.free_bufs_in_pool,
           poolinfo.number_of_bufs_shared,
           poolinfo.total_bufs_allocated_by_pool);
  }

  if (request->client_connected()) {
    if (!object_metadata->is_object_extended()) {
      // For normal object
      if (data_sent_to_client != requested_content_length) {
        read_object_data();
      } else {
        const auto mss = s3_timer.elapsed_time_in_millisec();
        LOG_PERF("get_object_send_data_ms", request_id.c_str(), mss);
        s3_stats_timing("get_object_send_data", mss);

        send_response_to_s3_client();
      }
    } else {
      // For fragmented object
      if (data_sent_to_client == requested_content_length) {
        const auto mss = s3_timer.elapsed_time_in_millisec();
        LOG_PERF("get_object_send_data_ms", request_id.c_str(), mss);
        s3_stats_timing("get_object_send_data", mss);
        send_response_to_s3_client();
        return;
      }

      if (data_sent_to_client_for_object !=
          extended_objects[next_fragment_object].requested_object_size) {
        read_object_data();
      } else {
        // Read next fragmented object
        next_fragment_object++;
        if (next_fragment_object < total_objects_to_read) {
          s3_log(S3_LOG_DEBUG, request_id,
                 "Read next object fragment at index (%u)\n",
                 next_fragment_object);
          read_fragmented_object();
        } else {
          send_response_to_s3_client();
        }
      }
    }
  } else {
    s3_log(S3_LOG_INFO, request_id,
           "Client disconnected. Aborting S3 GET operation\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::read_object_data_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Failed to read object data from motr\n");
  // set error only when reply is not started
  if (!read_object_reply_started) {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void S3GetObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id,
         "S3 request [%s] with total allocated mempool buffers = %zu\n",
         request_id.c_str(), request->get_mempool_buffer_count());

  if (reject_if_shutting_down()) {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      // Send response with 'Service Unavailable' code.
      s3_log(S3_LOG_DEBUG, request_id,
             "sending 'Service Unavailable' response...\n");
      S3Error error("ServiceUnavailable", request->get_request_id(),
                    request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->set_out_header_value("Content-Length",
                                    std::to_string(response_xml.length()));
      request->set_out_header_value("Retry-After", "1");
      // Dump request that failed
      s3_log(S3_LOG_ERROR, request_id,
             "S3 Get request failed. HTTP status code = %d\n",
             error.get_http_status_code());
      request->send_response(error.get_http_status_code(), response_xml);
    }
  } else if (is_error_state() && !get_s3_error_code().empty()) {
    // Invalid Bucket Name
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
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
    // Dump request that failed
    s3_log(S3_LOG_ERROR, request_id,
           "S3 Get request failed. HTTP status code = %d\n",
           error.get_http_status_code());
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata &&
             (object_metadata->get_content_length() == 0 ||
              (motr_reader &&
               motr_reader->get_state() == S3MotrReaderOpState::success))) {
    request->send_reply_end();
  } else {
    if (read_object_reply_started) {
      request->send_reply_end();
    } else {
      S3Error error("InternalError", request->get_request_id(),
                    request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->set_out_header_value("Content-Length",
                                    std::to_string(response_xml.length()));
      request->set_out_header_value("Retry-After", "1");
      // Dump request that failed
      s3_log(S3_LOG_ERROR, request_id,
             "S3 Get request failed. HTTP status code = %d\n",
             error.get_http_status_code());
      request->send_response(error.get_http_status_code(), response_xml);
    }
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetObjectAction::resume_action_step() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  // Free timer event object
  request->free_response_delay_timer(true);
  read_object_data();
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

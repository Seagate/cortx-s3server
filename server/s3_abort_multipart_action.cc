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

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <unistd.h>

#include "s3_abort_multipart_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_m0_uint128_helper.h"
#include "s3_common_utilities.h"

extern struct s3_motr_idx_layout global_probable_dead_object_list_index_layout;

S3AbortMultipartAction::S3AbortMultipartAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> s3_motr_apis,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_writer_factory,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3BucketAction(std::move(req), std::move(bucket_meta_factory), false) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  upload_id = request->get_query_string_value("uploadId");
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Abort Multipart API. Bucket[%s] \
         Object[%s] with UploadId[%s]\n",
         bucket_name.c_str(), object_name.c_str(), upload_id.c_str());

  action_uses_cleanup = true;

  s3_abort_mp_action_state = S3AbortMultipartActionState::empty;

  if (object_mp_meta_factory) {
    object_mp_metadata_factory = std::move(object_mp_meta_factory);
  } else {
    object_mp_metadata_factory =
        std::make_shared<S3ObjectMultipartMetadataFactory>();
  }

  if (motr_s3_writer_factory) {
    motr_writer_factory = std::move(motr_s3_writer_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  if (motr_s3_kvs_reader_factory) {
    motr_kvs_reader_factory = std::move(motr_s3_kvs_reader_factory);
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }

  if (part_meta_factory) {
    part_metadata_factory = std::move(part_meta_factory);
  } else {
    part_metadata_factory = std::make_shared<S3PartMetadataFactory>();
  }

  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  if (s3_motr_apis) {
    s3_motr_api = std::move(s3_motr_apis);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  setup_steps();
}

void S3AbortMultipartAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  ACTION_TASK_ADD(S3AbortMultipartAction::get_multipart_metadata, this);
  ACTION_TASK_ADD(S3AbortMultipartAction::get_next_parts, this);
  ACTION_TASK_ADD(
      S3AbortMultipartAction::add_parts_oids_to_probable_dead_oid_list, this);
  ACTION_TASK_ADD(S3AbortMultipartAction::delete_multipart_metadata, this);
  // TODO: delete_part_index_with_parts can also be done after send response
  ACTION_TASK_ADD(S3AbortMultipartAction::delete_part_index_with_parts, this);
  ACTION_TASK_ADD(S3AbortMultipartAction::send_response_to_s3_client, this);
  // ...
}

void S3AbortMultipartAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_abort_mp_action_state = S3AbortMultipartActionState::validationFailed;

  S3BucketMetadataState bucket_metadata_state = bucket_metadata->get_state();
  if (bucket_metadata_state == S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else if (bucket_metadata_state == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::get_multipart_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    multipart_index_layout = bucket_metadata->get_multipart_index_layout();

    if (zero(multipart_index_layout.oid)) {
      // There is no multipart upload to abort
      s3_abort_mp_action_state = S3AbortMultipartActionState::validationFailed;
      set_s3_error("NoSuchUpload");
      send_response_to_s3_client();
    } else {
      std::string multipart_key_name =
          request->get_object_name() + EXTENDED_METADATA_OBJECT_SEP + upload_id;
      object_multipart_metadata =
          object_mp_metadata_factory->create_object_mp_metadata_obj(
              request, multipart_index_layout, upload_id);
      object_multipart_metadata->rename_object_name(multipart_key_name);
      object_multipart_metadata->load(
          std::bind(&S3AbortMultipartAction::get_multipart_metadata_status,
                    this),
          std::bind(&S3AbortMultipartAction::get_multipart_metadata_status,
                    this));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::get_multipart_metadata_status() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::present) {
    if (object_multipart_metadata->get_upload_id() == upload_id) {
      next();
    } else {
      s3_abort_mp_action_state = S3AbortMultipartActionState::validationFailed;
      set_s3_error("NoSuchUpload");
      send_response_to_s3_client();
    }
  } else if (object_multipart_metadata->get_state() ==
             S3ObjectMetadataState::missing) {
    s3_abort_mp_action_state = S3AbortMultipartActionState::validationFailed;
    set_s3_error("NoSuchUpload");
    send_response_to_s3_client();
  } else {
    s3_abort_mp_action_state = S3AbortMultipartActionState::validationFailed;
    if (object_multipart_metadata->get_state() ==
        S3ObjectMetadataState::failed_to_launch) {
      s3_log(
          S3_LOG_ERROR, request_id,
          "Object metadata load operation failed due to pre launch failure\n");
      set_s3_error("ServiceUnavailable");
    } else {
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::get_next_parts() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Fetching next part listing\n");
  size_t count = S3Option::get_instance()->get_motr_idx_fetch_count();

  motr_kv_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  motr_kv_reader->next_keyval(
      object_multipart_metadata->get_part_index_layout(), last_key, count,
      std::bind(&S3AbortMultipartAction::get_next_parts_successful, this),
      std::bind(&S3AbortMultipartAction::get_next_parts_failed, this));
}

void S3AbortMultipartAction::get_next_parts_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }
  struct s3_motr_idx_layout part_index_layout = object_multipart_metadata->get_part_index_layout();
  bool atleast_one_json_error = false;
  auto& kvps = motr_kv_reader->get_key_values();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    s3_log(S3_LOG_DEBUG, request_id, "Read Object = %s\n", kv.first.c_str());
    auto part = part_metadata_factory->create_part_metadata_obj(
        request, part_index_layout, upload_id, atoi(kv.first.c_str()));

    if (part->from_json(kv.second.second) != 0) {
      atleast_one_json_error = true;
      s3_log(S3_LOG_ERROR, request_id,
             "Json Parsing failed. Index oid = "
             "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
             part_index_layout.oid.u_hi, part_index_layout.oid.u_lo,
             kv.first.c_str(), kv.second.second.c_str());
    } else {
      multipart_parts.push_back(part);
      s3_log(S3_LOG_DEBUG, request_id,
             "Part oid = "
             "%" SCNx64 " : %" SCNx64 ", layout_id = %d\n",
             part->get_oid().u_hi, part->get_oid().u_lo, part->get_layout_id());
    }

    if (--length == 0) {
      // this is the last element returned or we reached limit requested
      last_key = kv.first;
      break;
    }
  }
  if (atleast_one_json_error) {
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
  }
  // We ask for more if there is any.
  size_t count_we_requested =
      S3Option::get_instance()->get_motr_idx_fetch_count();
  if (kvps.size() < count_we_requested) {
    // Go ahead
    next();
  } else {
    get_next_parts();
  }
}

void S3AbortMultipartAction::get_next_parts_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Missing part listing\n");
    next();
    return;
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Part metadata next keyval operation failed due to pre launch "
           "failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to find part listing\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::add_parts_oids_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::map<std::string, std::string> probable_oid_list;

  if (multipart_parts.size() == 0) {
    s3_log(S3_LOG_DEBUG, request_id, "Parts not there\n");
    next();
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return;
  }

  for (auto& part_metadata : multipart_parts) {
    std::string part_oid_str = part_metadata->get_oid_str();

    // prepending a char depending on the size of the object (size based
    // bucketing of object)
    S3CommonUtilities::size_based_bucketing_of_objects(
        part_oid_str, part_metadata->get_content_length());
    std::unique_ptr<S3ProbableDeleteRecord> probable_del_rec;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding part probable del rec with key [%s]\n",
           part_oid_str.c_str());
    unsigned int part_no = stoi(part_metadata->get_part_number());
    // TODO -- Verify whether background delete process it appropriately
    probable_del_rec.reset(new S3ProbableDeleteRecord(
        part_oid_str, {0ULL, 0ULL}, part_metadata->get_part_number(),
        part_metadata->get_oid(), part_metadata->get_layout_id(),
        part_metadata->get_pvid_str(),
        part_metadata->get_part_index_layout().oid, {0ULL, 0ULL}, "",
        false /* force_delete */, true,
        part_metadata->get_part_index_layout().oid, 1, part_no, {0ULL, 0ULL},
        "",
        object_multipart_metadata->get_oid() /* parent oid of multipart */));
    probable_oid_list[probable_del_rec->get_key()] =
        probable_del_rec->to_json();
    probable_del_rec_list.push_back(std::move(probable_del_rec));
  }

  if (probable_oid_list.size() > 0) {
    if (!motr_kv_writer) {
      motr_kv_writer =
          mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
    }
    motr_kv_writer->put_keyval(
        global_probable_dead_object_list_index_layout, probable_oid_list,
        std::bind(&S3AbortMultipartAction::next, this),
        std::bind(&S3AbortMultipartAction::
                       add_parts_oids_to_probable_dead_oid_list_failed,
                  this));
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::add_parts_oids_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_abort_mp_action_state =
      S3AbortMultipartActionState::probableEntryRecordFailed;

  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_multipart_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  part_index_layout = object_multipart_metadata->get_part_index_layout();
  object_multipart_metadata->remove(
      std::bind(&S3AbortMultipartAction::delete_multipart_metadata_successful,
                this),
      std::bind(&S3AbortMultipartAction::delete_multipart_metadata_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_multipart_metadata_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_abort_mp_action_state = S3AbortMultipartActionState::uploadMetadataDeleted;
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_multipart_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id,
         "Object multipart meta data deletion failed\n");
  s3_abort_mp_action_state =
      S3AbortMultipartActionState::uploadMetadataDeleteFailed;

  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_part_index_with_parts() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (zero(part_index_layout.oid)) {
    next();
  } else {
    if (s3_fi_is_enabled("fail_remove_part_mindex")) {
      s3_fi_enable_once("motr_idx_delete_fail");
    }
    part_metadata = part_metadata_factory->create_part_metadata_obj(
        request, part_index_layout, upload_id, 1);
    part_metadata->remove_index(
        std::bind(
            &S3AbortMultipartAction::delete_part_index_with_parts_successful,
            this),
        std::bind(&S3AbortMultipartAction::delete_part_index_with_parts_failed,
                  this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_part_index_with_parts_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_abort_mp_action_state = S3AbortMultipartActionState::partsListIndexDeleted;
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_part_index_with_parts_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_abort_mp_action_state =
      S3AbortMultipartActionState::partsListIndexDeleteFailed;

  // Not checking part metatdata state for failed_to_launch here as we wont
  // return 503
  s3_log(S3_LOG_ERROR, request_id,
         "Failed to delete part index, this oid will be stale in Motr: "
         "%" SCNx64 " : %" SCNx64,
         part_index_layout.oid.u_hi, part_index_layout.oid.u_lo);
  // s3_iem(LOG_ERR, S3_IEM_DELETE_IDX_FAIL, S3_IEM_DELETE_IDX_FAIL_STR,
  //     S3_IEM_DELETE_IDX_FAIL_JSON);
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (is_error_state() && !get_s3_error_code().empty()) {
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
    s3_abort_mp_action_state = S3AbortMultipartActionState::completed;
    request->send_response(S3HttpSuccess200);
  }
  startcleanup();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::startcleanup() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;

  if (s3_abort_mp_action_state == S3AbortMultipartActionState::empty ||
      s3_abort_mp_action_state ==
          S3AbortMultipartActionState::validationFailed ||
      s3_abort_mp_action_state ==
          S3AbortMultipartActionState::probableEntryRecordFailed) {
    s3_log(S3_LOG_DEBUG, request_id,
           "No Cleanup required: s3_abort_mp_action_state[%d]\n",
           s3_abort_mp_action_state);
    // Nothing for clean up
    done();
  } else {
    if (s3_abort_mp_action_state ==
        S3AbortMultipartActionState::uploadMetadataDeleteFailed) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Abort Failed. s3_abort_mp_action_state = "
             "uploadMetadataDeleteFailed\n");
      // Failed to delete metadata, so object is still can become live with
      // complete operation, remove probable rec
      ACTION_TASK_ADD(S3AbortMultipartAction::remove_probable_records, this);
    } else {
      s3_log(S3_LOG_DEBUG, request_id,
             "Cleanup the object: s3_abort_mp_action_state[%d]\n",
             s3_abort_mp_action_state);
      // Metadata deleted, so object is gone for S3 client, clean storage.
      ACTION_TASK_ADD(S3AbortMultipartAction::mark_oids_for_deletion, this);
      ACTION_TASK_ADD(S3AbortMultipartAction::delete_part, this);
      // If delete object is successful and part list deleted,
      // attempt to delete probable record
    }
    // Start running the cleanup task list
    start();
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::mark_oids_for_deletion() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  assert(!oid_str.empty());
  if (probable_del_rec_list.size() == 0) {
    next();
    return;
  }

  std::map<std::string, std::string> oid_key_val_map;
  for (auto& delete_rec : probable_del_rec_list) {
    delete_rec->set_force_delete(true);
    oid_key_val_map[delete_rec->get_key()] = delete_rec->to_json();
  }

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_layout,
                             oid_key_val_map,
                             std::bind(&S3AbortMultipartAction::next, this),
                             std::bind(&S3AbortMultipartAction::next, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_part() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // process to delete object
  if (multipart_parts.size() == 0) {
    next();
    return;
  }
  if (!motr_writer) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }

  auto& part_metadata = multipart_parts.back();
  struct m0_uint128 id = part_metadata->get_oid();
  motr_writer->set_oid(id);
  motr_writer->set_layout_id(part_metadata->get_layout_id());
  motr_writer->delete_object(
      std::bind(&S3AbortMultipartAction::delete_parts_successful, this),
      std::bind(&S3AbortMultipartAction::next, this),
      id,
      part_metadata->get_layout_id(),
      object_multipart_metadata->get_pvid());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::delete_parts_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (multipart_parts.size() == 0) {
    return;
  }
  s3_log(S3_LOG_INFO, request_id,
         "Deleted old object oid "
         "[%" SCNx64 " : %" SCNx64 "]",
         (motr_writer->get_oid()).u_hi, (motr_writer->get_oid()).u_lo);
  multipart_parts.erase(multipart_parts.end());

  if (multipart_parts.size() > 0) {
    delete_part();
  } else {
    remove_probable_records();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3AbortMultipartAction::remove_probable_records() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::vector<std::string> keys_to_delete;
  assert(!oid_str.empty());

  if (s3_abort_mp_action_state ==
      S3AbortMultipartActionState::partsListIndexDeleted) {
    // Delete probable record only if object and part list is deleted.
    if (!motr_kv_writer) {
      motr_kv_writer =
          mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
    }
    for (auto& probable_rec : probable_del_rec_list) {
      keys_to_delete.push_back(probable_rec->get_key());
    }
    if (!motr_kv_writer) {
      motr_kv_writer =
          mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
    }
    motr_kv_writer->delete_keyval(
        global_probable_dead_object_list_index_layout, keys_to_delete,
        std::bind(&S3AbortMultipartAction::next, this),
        std::bind(&S3AbortMultipartAction::next, this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

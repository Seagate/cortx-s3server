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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 6-Jan-2016
 */

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <unistd.h>

#include "s3_abort_multipart_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_m0_uint128_helper.h"

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3AbortMultipartAction::S3AbortMultipartAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<ClovisAPI> s3_clovis_apis,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3ClovisWriterFactory> clovis_s3_writer_factory,
    std::shared_ptr<S3ClovisKVSReaderFactory> clovis_s3_kvs_reader_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory)
    : S3BucketAction(std::move(req), std::move(bucket_meta_factory), false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  upload_id = request->get_query_string_value("uploadId");
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: Abort Multipart API. Bucket[%s] \
         Object[%s] with UploadId[%s]\n",
         bucket_name.c_str(), object_name.c_str(), upload_id.c_str());

  multipart_oid = {0ULL, 0ULL};
  part_index_oid = {0ULL, 0ULL};

  if (object_mp_meta_factory) {
    object_mp_metadata_factory = std::move(object_mp_meta_factory);
  } else {
    object_mp_metadata_factory =
        std::make_shared<S3ObjectMultipartMetadataFactory>();
  }

  if (clovis_s3_writer_factory) {
    clovis_writer_factory = std::move(clovis_s3_writer_factory);
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }

  if (clovis_s3_kvs_reader_factory) {
    clovis_kvs_reader_factory = std::move(clovis_s3_kvs_reader_factory);
  } else {
    clovis_kvs_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  if (part_meta_factory) {
    part_metadata_factory = std::move(part_meta_factory);
  } else {
    part_metadata_factory = std::make_shared<S3PartMetadataFactory>();
  }

  if (kv_writer_factory) {
    clovis_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    clovis_kv_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  if (s3_clovis_apis) {
    s3_clovis_api = std::move(s3_clovis_apis);
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }
  setup_steps();
}

void S3AbortMultipartAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  ACTION_TASK_ADD(S3AbortMultipartAction::get_multipart_metadata, this);
  if (S3Option::get_instance()->is_s3server_objectleak_tracking_enabled()) {
    ACTION_TASK_ADD(
        S3AbortMultipartAction::add_object_oid_to_probable_dead_oid_list, this);
  }
  ACTION_TASK_ADD(S3AbortMultipartAction::delete_multipart_metadata, this);
  ACTION_TASK_ADD(S3AbortMultipartAction::delete_part_index_with_parts, this);
  ACTION_TASK_ADD(S3AbortMultipartAction::send_response_to_s3_client, this);
  // ...
}

void S3AbortMultipartAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::get_multipart_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    multipart_oid = bucket_metadata->get_multipart_index_oid();
    if (multipart_oid.u_lo == 0ULL && multipart_oid.u_hi == 0ULL) {
      // There is no multipart upload to abort
      set_s3_error("NoSuchUpload");
      send_response_to_s3_client();
    } else {
      object_multipart_metadata =
          object_mp_metadata_factory->create_object_mp_metadata_obj(
              request, multipart_oid, true, upload_id);

      object_multipart_metadata->load(
          std::bind(&S3AbortMultipartAction::get_multipart_metadata_status,
                    this),
          std::bind(&S3AbortMultipartAction::get_multipart_metadata_status,
                    this));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::get_multipart_metadata_status() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::present) {
    if (object_multipart_metadata->get_upload_id() == upload_id) {
      next();
    } else {
      set_s3_error("NoSuchUpload");
      send_response_to_s3_client();
    }
  } else if (object_multipart_metadata->get_state() ==
             S3ObjectMetadataState::missing) {
    set_s3_error("NoSuchUpload");
    send_response_to_s3_client();
  } else {
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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  probable_oid_list.clear();
  m0_uint128 object_oid = object_multipart_metadata->get_oid();
  // store current object oid
  if (object_oid.u_hi || object_oid.u_lo) {
    std::string old_oid_str = S3M0Uint128Helper::to_string(object_oid);
    probable_oid_list[S3M0Uint128Helper::to_string(object_oid)] =
        object_multipart_metadata->create_probable_delete_record(
            object_multipart_metadata->get_layout_id());
  }

  if (!probable_oid_list.empty()) {
    if (!clovis_kv_writer) {
      clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
          request, s3_clovis_api);
    }
    clovis_kv_writer->put_keyval(
        global_probable_dead_object_list_index_oid, probable_oid_list,
        std::bind(&S3AbortMultipartAction::next, this),
        std::bind(&S3AbortMultipartAction::
                       add_object_oid_to_probable_dead_oid_list_failed,
                  this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::delete_multipart_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  part_index_oid = object_multipart_metadata->get_part_index_oid();
  object_multipart_metadata->remove(
      std::bind(&S3AbortMultipartAction::next, this),
      std::bind(&S3AbortMultipartAction::delete_multipart_metadata_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::delete_multipart_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id,
         "Object multipart meta data deletion failed\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::delete_part_index_with_parts() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (part_index_oid.u_lo == 0ULL && part_index_oid.u_hi == 0ULL) {
    next();
  } else {
    if (s3_fi_is_enabled("fail_remove_part_mindex")) {
      s3_fi_enable_once("clovis_idx_delete_fail");
    }
    part_metadata = part_metadata_factory->create_part_metadata_obj(
        request, part_index_oid, upload_id, 1);
    part_metadata->remove_index(
        std::bind(&S3AbortMultipartAction::next, this),
        std::bind(&S3AbortMultipartAction::delete_part_index_with_parts_failed,
                  this));
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::delete_part_index_with_parts_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // Not checking part metatdata state for failed_to_launch here as we wont
  // return 503
  s3_log(S3_LOG_ERROR, request_id,
         "Failed to delete part index, this oid will be stale in Mero: "
         "%" SCNx64 " : %" SCNx64,
         part_index_oid.u_hi, part_index_oid.u_lo);
  s3_iem(LOG_ERR, S3_IEM_DELETE_IDX_FAIL, S3_IEM_DELETE_IDX_FAIL_STR,
         S3_IEM_DELETE_IDX_FAIL_JSON);
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (is_error_state() && !get_s3_error_code().empty()) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code().compare("ServiceUnavailable")) {
      request->set_out_header_value("Retry-After", "1");
    }
    request->send_response(error.get_http_status_code(), response_xml);
  } else {
    request->send_response(S3HttpSuccess200);
  }
  cleanup();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::cleanup() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // proceed to delete new object uploaded during multipart, on object multipart
  // metadata success only
  if (!is_error_state() && get_s3_error_code().empty() &&
      object_multipart_metadata) {
    m0_uint128 object_oid = object_multipart_metadata->get_oid();

    // process to delete object for newly uploaded object
    if (object_oid.u_hi || object_oid.u_lo) {
      int layout_id = object_multipart_metadata->get_layout_id();
      if (!clovis_writer) {
        clovis_writer =
            clovis_writer_factory->create_clovis_writer(request, object_oid);
      }
      clovis_writer->set_oid(object_oid);
      clovis_writer->delete_object(
          std::bind(&S3AbortMultipartAction::cleanup_successful, this),
          std::bind(&S3AbortMultipartAction::cleanup_failed, this), layout_id);
    } else {
      cleanup_oid_from_probable_dead_oid_list();
    }
  } else {
    cleanup_oid_from_probable_dead_oid_list();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::cleanup_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::cleanup_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // on multipart operation: multi_part metadata deletion succesful, but failed
  // to delete object oid. so erase the old object oid from probable_oid_list
  // map and this old object oid will be retained in
  // global_probable_dead_object_list_index_oid
  m0_uint128 object_oid = object_multipart_metadata->get_oid();
  probable_oid_list.erase(S3M0Uint128Helper::to_string(object_oid));
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AbortMultipartAction::cleanup_oid_from_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (!probable_oid_list.empty()) {
    std::vector<std::string>
        clean_valid_oid_from_probable_dead_object_list_index;

    // final probable_oid_list map will have valid object oids that needs to be
    // cleanup from global_probable_dead_object_list_index_oid
    for (auto& kv : probable_oid_list) {
      clean_valid_oid_from_probable_dead_object_list_index.push_back(kv.first);
    }

    if (!clean_valid_oid_from_probable_dead_object_list_index.empty()) {
      if (!clovis_kv_writer) {
        clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
            request, s3_clovis_api);
      }
      clovis_kv_writer->delete_keyval(
          global_probable_dead_object_list_index_oid,
          clean_valid_oid_from_probable_dead_object_list_index,
          std::bind(&Action::done, this), std::bind(&Action::done, this));
    } else {
      done();
    }
  } else {
    done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

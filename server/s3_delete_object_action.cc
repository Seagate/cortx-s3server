/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original creation date: 1-Oct-2015
 */

#include "s3_delete_object_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_m0_uint128_helper.h"

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3DeleteObjectAction::S3DeleteObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3ClovisWriterFactory> writer_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<ClovisAPI> clovis_api)
    : S3Action(req, false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: Delete Object API. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  if (clovis_api) {
    s3_clovis_api = clovis_api;
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  if (object_meta_factory) {
    object_metadata_factory = object_meta_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }

  if (writer_factory) {
    clovis_writer_factory = writer_factory;
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }

  if (kv_writer_factory) {
    clovis_kv_writer_factory = kv_writer_factory;
  } else {
    clovis_kv_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  setup_steps();
}

void S3DeleteObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  add_task(std::bind(&S3DeleteObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3DeleteObjectAction::fetch_object_info, this));
  // We delete metadata first followed by object, since if we delete
  // the object first and say for some reason metadata clean up fails,
  // If this "oid" gets allocated to some other object, current object
  // metadata will point to someone else's object leading 2 problems:
  // accessing someone else's object and second retrying delete, deleting
  // someone else's object. Whereas deleting metadata first will worst case
  // lead to object leak in mero which can handle separately.
  // To delete stale objects: ref: MINT-602
  if (S3Option::get_instance()->is_s3server_objectleak_tracking_enabled()) {
    add_task(std::bind(
        &S3DeleteObjectAction::add_object_oid_to_probable_dead_oid_list, this));
  }
  add_task(std::bind(&S3DeleteObjectAction::delete_metadata, this));
  add_task(std::bind(&S3DeleteObjectAction::send_response_to_s3_client, this));
  // ...
}

void S3DeleteObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(
      std::bind(&S3DeleteObjectAction::next, this),
      std::bind(&S3DeleteObjectAction::fetch_bucket_metadata_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::fetch_bucket_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  S3BucketMetadataState bucket_metadata_state = bucket_metadata->get_state();
  if (bucket_metadata_state == S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else if (bucket_metadata_state == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata_state == S3BucketMetadataState::present) {
    set_s3_error("AccessDenied");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::fetch_object_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    struct m0_uint128 object_list_indx_oid =
        bucket_metadata->get_object_list_index_oid();
    if (object_list_indx_oid.u_hi == 0ULL &&
        object_list_indx_oid.u_lo == 0ULL) {
      // There is no object list index, hence object doesn't exist
      s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
      send_response_to_s3_client();
    } else {
      object_metadata = object_metadata_factory->create_object_metadata_obj(
          request, object_list_indx_oid);

      object_metadata->load(
          std::bind(&S3DeleteObjectAction::next, this),
          std::bind(&S3DeleteObjectAction::fetch_object_info_failed, this));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::fetch_object_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
      s3_log(S3_LOG_WARN, request_id, "Object not found\n");
    } else if (object_metadata->get_state() ==
               S3ObjectMetadataState::failed_to_launch) {
      s3_log(
          S3_LOG_ERROR, request_id,
          "Object metadata load operation failed due to pre launch failure\n");
      set_s3_error("ServiceUnavailable");
    } else {
      s3_log(S3_LOG_WARN, request_id, "Failed to look up Object metadata\n");
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::string oid_str =
      S3M0Uint128Helper::to_string(object_metadata->get_oid());

  clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
      request, s3_clovis_api);
  clovis_kv_writer->put_keyval(
      global_probable_dead_object_list_index_oid, oid_str,
      object_metadata->create_probable_delete_record(
          object_metadata->get_layout_id()),
      std::bind(&S3DeleteObjectAction::next, this),
      std::bind(&S3DeleteObjectAction::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  set_s3_error("InternalError");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::delete_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  object_metadata->remove(
      std::bind(&S3DeleteObjectAction::next, this),
      std::bind(&S3DeleteObjectAction::delete_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::delete_metadata_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed to delete Object metadata\n");
  set_s3_error("InternalError");
  send_response_to_s3_client();
}

void S3DeleteObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (is_error_state() && !get_s3_error_code().empty()) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }

    request->send_response(error.get_http_status_code(), response_xml);
  } else {
    request->send_response(S3HttpSuccess204);
  }
  cleanup();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::cleanup() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if ((object_metadata != nullptr) &&
      (object_metadata->get_state() == S3ObjectMetadataState::deleted)) {
    // process to delete object
    clovis_writer = clovis_writer_factory->create_clovis_writer(
        request, object_metadata->get_oid());
    clovis_writer->delete_object(
        std::bind(
            &S3DeleteObjectAction::cleanup_oid_from_probable_dead_oid_list,
            this),
        std::bind(&Action::done, this), object_metadata->get_layout_id());
  } else {
    // metadata failed to delete, so remove the entry from
    // probable_dead_oid_list
    cleanup_oid_from_probable_dead_oid_list();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::cleanup_oid_from_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_metadata != nullptr &&
      S3Option::get_instance()->is_s3server_objectleak_tracking_enabled()) {
    m0_uint128 old_oid = object_metadata->get_oid();
    if (old_oid.u_hi != 0ULL || old_oid.u_lo != 0ULL) {
      std::string oid_str = S3M0Uint128Helper::to_string(old_oid);
      if (clovis_kv_writer == nullptr) {
        clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
            request, s3_clovis_api);
      }
      clovis_kv_writer->delete_keyval(
          global_probable_dead_object_list_index_oid, oid_str,
          std::bind(&Action::done, this), std::bind(&Action::done, this));
    } else {
      done();
    }
  } else {
    done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

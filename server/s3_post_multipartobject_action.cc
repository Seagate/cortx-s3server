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

#include "s3_post_multipartobject_action.h"
#include "s3_clovis_layout.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_stats.h"
#include "s3_uri_to_mero_oid.h"

S3PostMultipartObjectAction::S3PostMultipartObjectAction(
    std::shared_ptr<S3RequestObject> req,
    S3BucketMetadataFactory* bucket_meta_factory,
    S3ObjectMultipartMetadataFactory* object_mp_meta_factory,
    S3ObjectMetadataFactory* object_meta_factory,
    S3PartMetadataFactory* part_meta_factory,
    S3ClovisWriterFactory* clovis_s3_factory,
    std::shared_ptr<ClovisAPI> clovis_api)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: Initiate Multipart Upload. Bucket[%s]\
         Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  if (clovis_api) {
    s3_clovis_api = clovis_api;
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }
  oid = {0ULL, 0ULL};
  S3UriToMeroOID(s3_clovis_api, request->get_object_uri().c_str(), request_id,
                 &oid);

  tried_count = 0;

  old_oid = {0ULL, 0ULL};
  old_layout_id = -1;

  // Since we cannot predict the object size during multipart init, we use the
  // best recommended layout for better Performance
  layout_id =
      S3ClovisLayoutMap::get_instance()->get_best_layout_for_object_size();

  bucket_metadata = nullptr;
  object_metadata = nullptr;
  object_multipart_metadata = nullptr;
  part_metadata = nullptr;
  clovis_writer = nullptr;

  multipart_index_oid = {0ULL, 0ULL};
  salt = "uri_salt_";
  if (bucket_meta_factory) {
    bucket_metadata_factory.reset(bucket_meta_factory);
  } else {
    bucket_metadata_factory.reset(new S3BucketMetadataFactory());
  }

  if (object_meta_factory) {
    object_metadata_factory.reset(object_meta_factory);
  } else {
    object_metadata_factory.reset(new S3ObjectMetadataFactory());
  }

  if (object_mp_meta_factory) {
    object_mp_metadata_factory.reset(object_mp_meta_factory);
  } else {
    object_mp_metadata_factory.reset(new S3ObjectMultipartMetadataFactory());
  }

  if (clovis_s3_factory) {
    clovis_writer_factory.reset(clovis_s3_factory);
  } else {
    clovis_writer_factory.reset(new S3ClovisWriterFactory());
  }

  if (part_meta_factory) {
    part_metadata_factory.reset(part_meta_factory);
  } else {
    part_metadata_factory.reset(new S3PartMetadataFactory());
  }

  setup_steps();
}

void S3PostMultipartObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3PostMultipartObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3PostMultipartObjectAction::check_upload_is_inprogress,
                     this));
  add_task(std::bind(&S3PostMultipartObjectAction::fetch_object_info, this));
  add_task(std::bind(&S3PostMultipartObjectAction::create_object, this));
  add_task(
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index, this));
  add_task(std::bind(&S3PostMultipartObjectAction::save_upload_metadata, this));
  add_task(
      std::bind(&S3PostMultipartObjectAction::save_multipart_metadata, this));
  add_task(std::bind(&S3PostMultipartObjectAction::send_response_to_s3_client,
                     this));
  // ...
}

void S3PostMultipartObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);

  bucket_metadata->load(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::fetch_bucket_info_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    set_s3_error("AccessDenied");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::check_upload_is_inprogress() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    S3Uuid uuid;
    upload_id = uuid.get_string_uuid();
    multipart_index_oid = bucket_metadata->get_multipart_index_oid();
    object_multipart_metadata =
        object_mp_metadata_factory->create_object_mp_metadata_obj(
            request, multipart_index_oid, true, upload_id);
    if (multipart_index_oid.u_lo == 0ULL && multipart_index_oid.u_hi == 0ULL) {
      // There is no multipart in progress, move on
      next();
    } else {
      object_multipart_metadata->load(
          std::bind(&S3PostMultipartObjectAction::next, this),
          std::bind(&S3PostMultipartObjectAction::next, this));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::fetch_object_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_multipart_metadata->get_state() !=
      S3ObjectMetadataState::present) {
    if (object_multipart_metadata->get_state() ==
        S3ObjectMetadataState::failed_to_launch) {
      s3_log(
          S3_LOG_ERROR, request_id,
          "Object metadata load operation failed due to pre launch failure\n");
      set_s3_error("ServiceUnavailable");
      send_response_to_s3_client();
      s3_log(S3_LOG_DEBUG, "", "Exiting\n");
      return;
    }
    s3_log(S3_LOG_DEBUG, request_id, "Found bucket metadata\n");
    object_list_oid = bucket_metadata->get_object_list_index_oid();
    if (object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL) {
      s3_log(S3_LOG_DEBUG, request_id, "No existing object, Create it.\n");
      next();
    } else {
      object_metadata = object_metadata_factory->create_object_metadata_obj(
          request, object_list_oid);

      object_metadata->load(
          std::bind(&S3PostMultipartObjectAction::fetch_object_info_status,
                    this),
          std::bind(&S3PostMultipartObjectAction::next, this));
    }
  } else {
    //++
    // Bailing out in case if the multipart upload is already in progress,
    // this will ensure that object doesn't go to inconsistent state

    // Once multipart is taken care in mero, it may not be needed
    // Note - Currently as per aws doc.
    // (http://docs.aws.amazon.com/AmazonS3/latest/API/mpUploadListMPUpload.html)
    // multipart upload details
    // of the same key initiated at different times is allowed.
    //--
    set_s3_error("InvalidObjectState");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::fetch_object_info_status() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  S3ObjectMetadataState object_state = object_metadata->get_state();
  if (object_state == S3ObjectMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "S3ObjectMetadataState::present\n");
    old_oid = object_metadata->get_oid();
    old_layout_id = object_metadata->get_layout_id();
    s3_log(S3_LOG_DEBUG, request_id,
           "Object with oid "
           "%" SCNx64 " : %" SCNx64 " already exists, creating new oid\n",
           old_oid.u_hi, old_oid.u_lo);
    create_new_oid(old_oid);

    object_multipart_metadata->set_old_oid(old_oid);
    object_multipart_metadata->set_old_layout_id(old_layout_id);
    next();
  } else if (object_state == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "S3ObjectMetadataState::missing\n");
    next();
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to look up metadata.\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  create_object_timer.start();
  if (tried_count == 0) {
    clovis_writer = clovis_writer_factory->create_clovis_writer(request, oid);
  } else {
    clovis_writer->set_oid(oid);
  }

  clovis_writer->create_object(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::create_object_failed, this),
      layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "post_multipartobject_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  create_object_timer.stop();
  LOG_PERF("create_object_failed_ms",
           create_object_timer.elapsed_time_in_millisec());
  s3_stats_timing("multipart_create_object_failed",
                  create_object_timer.elapsed_time_in_millisec());

  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
      collision_occured();
  } else if (clovis_writer->get_state() ==
             S3ClovisWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
    // Any other error report failure.
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::collision_occured() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (tried_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id, "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid(oid);
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, request_id,
             "Object ID collision happened %d times for uri %s\n", tried_count,
             request->get_object_uri().c_str());
    }
    create_object();
  } else {
    if (tried_count > MAX_COLLISION_RETRY_COUNT) {
      s3_log(S3_LOG_ERROR, request_id,
             "Failed to resolve object id collision %d times for uri %s\n",
             tried_count, request->get_object_uri().c_str());
      s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
             S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
}

void S3PostMultipartObjectAction::create_new_oid(
    struct m0_uint128 current_oid) {
  int salt_counter = 0;
  std::string salted_uri;
  do {
    salted_uri = request->get_object_uri() + salt +
                 std::to_string(salt_counter) + std::to_string(tried_count);

    S3UriToMeroOID(s3_clovis_api, salted_uri.c_str(), request_id, &oid);
    ++salt_counter;
  } while ((oid.u_hi == current_oid.u_hi) && (oid.u_lo == current_oid.u_lo));

  return;
}

void S3PostMultipartObjectAction::rollback_create() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  clovis_writer->set_oid(oid);
  clovis_writer->delete_object(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(&S3PostMultipartObjectAction::rollback_create_failed, this),
      layout_id);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_create_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    rollback_next();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Deletion of object failed, this oid will be stale in Mero: "
           "%" SCNx64 " : %" SCNx64 "\n",
           oid.u_hi, oid.u_lo);
    s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
           S3_IEM_DELETE_OBJ_FAIL_JSON);
    if (clovis_writer->get_state() == S3ClovisWriterOpState::failed_to_launch) {
      set_s3_error("ServiceUnavailable");
    }
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_create_part_meta_index() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  part_metadata->remove_index(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(
          &S3PostMultipartObjectAction::rollback_create_part_meta_index_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_create_part_meta_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  m0_uint128 part_index_oid = part_metadata->get_part_index_oid();
  s3_log(S3_LOG_WARN, request_id, "Deletion of part metadata failed\n");
  if (part_metadata->get_state() == S3PartMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  }
  s3_log(S3_LOG_ERROR, request_id,
         "Deletion of index failed, this oid will be stale in Mero"
         "%" SCNx64 " : %" SCNx64 "\n",
         part_index_oid.u_hi, part_index_oid.u_lo);
  s3_iem(LOG_ERR, S3_IEM_DELETE_IDX_FAIL, S3_IEM_DELETE_IDX_FAIL_STR,
         S3_IEM_DELETE_IDX_FAIL_JSON);
  rollback_done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::save_upload_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  create_object_timer.stop();
  LOG_PERF("create_object_successful_ms",
           create_object_timer.elapsed_time_in_millisec());
  s3_stats_timing("multipart_create_object_success",
                  create_object_timer.elapsed_time_in_millisec());

  // mark rollback point
  add_task_rollback(std::bind(
      &S3PostMultipartObjectAction::rollback_create_part_meta_index, this));

  object_multipart_metadata->set_layout_id(layout_id);

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      object_multipart_metadata->add_user_defined_attribute(it.first,
                                                            it.second);
    }
  }
  object_multipart_metadata->set_oid(oid);
  object_multipart_metadata->set_part_index_oid(
      part_metadata->get_part_index_oid());
  object_multipart_metadata->save(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::save_upload_metadata_failed,
                this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "post_multipartobject_action_save_upload_metadata_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::save_upload_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id, "save upload index metadata failed\n");
    set_s3_error("ServiceUnavailable");
  }
  // Trigger rollback to undo changes done and report error
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_upload_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  object_multipart_metadata->remove(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(&S3PostMultipartObjectAction::rollback_upload_metadata_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_upload_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::missing) {
    rollback_next();
  } else {
    // Log rollback failure.
    s3_log(S3_LOG_WARN, request_id, "Deletion of multipart metadata failed\n");
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_part_meta_index() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // mark rollback point
  add_task_rollback(
      std::bind(&S3PostMultipartObjectAction::rollback_create, this));
  part_metadata =
      part_metadata_factory->create_part_metadata_obj(request, upload_id, 0);
  part_metadata->create_index(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_part_meta_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (part_metadata->get_state() == S3PartMetadataState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id, "Multipart index creation failed\n");
    set_s3_error("ServiceUnavailable");
  }
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::save_multipart_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  if (multipart_index_oid.u_lo == 0 && multipart_index_oid.u_hi == 0) {
    // multipart index created, set it
    bucket_metadata->set_multipart_index_oid(
        object_multipart_metadata->get_index_oid());
    // mark rollback point
    add_task_rollback(std::bind(
        &S3PostMultipartObjectAction::rollback_upload_metadata, this));
    bucket_metadata->save(
        std::bind(&S3PostMultipartObjectAction::next, this),
        std::bind(&S3PostMultipartObjectAction::save_multipart_metadata_failed,
                  this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::save_multipart_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id,
         "Failed to save multipart index oid metadata\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_WARN, request_id, "Bucket multipart metadata save failed\n");
    set_s3_error("ServiceUnavailable");
  }
  // Trigger rollback to undo changes done and report error
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
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
  } else if ((object_multipart_metadata->get_state() ==
              S3ObjectMetadataState::saved) &&
             ((part_metadata->get_state() ==
               S3PartMetadataState::store_created) ||
              (part_metadata->get_state() == S3PartMetadataState::present))) {
    std::string response;
    std::string object_name = request->get_object_name();
    std::string bucket_name = request->get_bucket_name();
    response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    response += "<InitiateMultipartUploadResult xmlns=\"http://s3\">\n";
    response += "<Bucket>" + bucket_name + "</Bucket>\n";
    response += "<Key>" + object_name + "</Key>\n";
    response += "<UploadId>" + upload_id + "</UploadId>";
    response += "</InitiateMultipartUploadResult>";
    request->set_out_header_value("UploadId", upload_id);

    request->send_response(S3HttpSuccess200, response);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}

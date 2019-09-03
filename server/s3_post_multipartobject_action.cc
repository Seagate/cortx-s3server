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
#include "s3_m0_uint128_helper.h"
#include "s3_stats.h"
#include "s3_uri_to_mero_oid.h"
#include "s3_post_multipartobject_action.h"
#include "s3_m0_uint128_helper.h"
#include <evhttp.h>

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3PostMultipartObjectAction::S3PostMultipartObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3ObjectMultipartMetadataFactory> object_mp_meta_factory,
    std::shared_ptr<S3PartMetadataFactory> part_meta_factory,
    std::shared_ptr<S3ClovisWriterFactory> clovis_s3_factory,
    std::shared_ptr<S3PutTagsBodyFactory> put_tags_body_factory,
    std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req)) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: Initiate Multipart Upload. Bucket[%s]\
         Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  if (clovis_api) {
    s3_clovis_api = std::move(clovis_api);
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

  multipart_index_oid = {0ULL, 0ULL};
  salt = "uri_salt_";

  if (object_mp_meta_factory) {
    object_mp_metadata_factory = std::move(object_mp_meta_factory);
  } else {
    object_mp_metadata_factory =
        std::make_shared<S3ObjectMultipartMetadataFactory>();
  }

  if (clovis_s3_factory) {
    clovis_writer_factory = std::move(clovis_s3_factory);
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }

  if (part_meta_factory) {
    part_metadata_factory = std::move(part_meta_factory);
  } else {
    part_metadata_factory = std::make_shared<S3PartMetadataFactory>();
  }
  if (put_tags_body_factory) {
    put_object_tag_body_factory = std::move(put_tags_body_factory);
  } else {
    put_object_tag_body_factory = std::make_shared<S3PutTagsBodyFactory>();
  }

  if (kv_writer_factory) {
    clovis_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    clovis_kv_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  setup_steps();
}

void S3PostMultipartObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");

  if (!request->get_header_value("x-amz-tagging").empty()) {

    add_task(std::bind(
        &S3PostMultipartObjectAction::validate_x_amz_tagging_if_present, this));
  }
  add_task(std::bind(&S3PostMultipartObjectAction::check_upload_is_inprogress,
                     this));
  add_task(std::bind(&S3PostMultipartObjectAction::create_object, this));
  if (S3Option::get_instance()->is_s3server_objectleak_tracking_enabled()) {
    add_task(std::bind(
        &S3PostMultipartObjectAction::add_object_oid_to_probable_dead_oid_list,
        this));
  }
  add_task(
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index, this));
  add_task(std::bind(&S3PostMultipartObjectAction::save_upload_metadata, this));
  add_task(
      std::bind(&S3PostMultipartObjectAction::save_multipart_metadata, this));
  add_task(std::bind(&S3PostMultipartObjectAction::send_response_to_s3_client,
                     this));
  // ...
}
void S3PostMultipartObjectAction::fetch_object_info_failed() { next(); }
void S3PostMultipartObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
void S3PostMultipartObjectAction::fetch_object_info_success() { next(); }

void S3PostMultipartObjectAction::validate_x_amz_tagging_if_present() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::string new_object_tags = request->get_header_value("x-amz-tagging");
  s3_log(S3_LOG_DEBUG, request_id, "Received tags= %s\n",
         new_object_tags.c_str());
  if (!new_object_tags.empty()) {
    parse_x_amz_tagging_header(new_object_tags);
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
}

void S3PostMultipartObjectAction::parse_x_amz_tagging_header(
    std::string content) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  struct evkeyvalq key_value;
  memset(&key_value, 0, sizeof(key_value));
  if (0 == evhttp_parse_query_str(content.c_str(), &key_value)) {
    char* decoded_key = NULL;
    for (struct evkeyval* header = key_value.tqh_first; header;
         header = header->next.tqe_next) {

      decoded_key = evhttp_decode_uri(header->key);
      s3_log(S3_LOG_DEBUG, request_id,
             "Successfully parsed the Key Values=%s %s", decoded_key,
             header->value);
      new_object_tags_map[decoded_key] = header->value;
    }
    validate_tags();
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::validate_tags() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  std::string xml;
  std::shared_ptr<S3PutTagBody> put_object_tag_body =
      put_object_tag_body_factory->create_put_resource_tags_body(xml,
                                                                 request_id);

  if (put_object_tag_body->validate_object_xml_tags(new_object_tags_map)) {
    next();
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
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
      // object_list_oid is null only when bucket metadata is corrupted.
      // user has to delete and recreate the bucket again to make it work.
      s3_log(S3_LOG_ERROR, request_id, "Bucket(%s) metadata is corrupted.\n",
             request->get_bucket_name().c_str());
      s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
             S3_IEM_METADATA_CORRUPTED_JSON);
      set_s3_error("MetaDataCorruption");
      send_response_to_s3_client();
    } else {
      object_multipart_metadata->load(
          std::bind(
              &S3PostMultipartObjectAction::check_multipart_object_info_status,
              this),
          std::bind(
              &S3PostMultipartObjectAction::check_multipart_object_info_status,
              this));
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::check_multipart_object_info_status() {
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
      // object_list_oid is null only when bucket metadata is corrupted.
      // user has to delete and recreate the bucket again to make it work.
      s3_log(S3_LOG_ERROR, request_id, "Bucket(%s) metadata is corrupted.\n",
             request->get_bucket_name().c_str());
      s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
             S3_IEM_METADATA_CORRUPTED_JSON);
      set_s3_error("MetaDataCorruption");
      send_response_to_s3_client();
    } else {
      check_object_state();
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

void S3PostMultipartObjectAction::check_object_state() {
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
      std::bind(&S3PostMultipartObjectAction::create_object_successful, this),
      std::bind(&S3PostMultipartObjectAction::create_object_failed, this),
      layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "post_multipartobject_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  create_object_timer.stop();
  const size_t time_in_millisecond =
      create_object_timer.elapsed_time_in_millisec();
  LOG_PERF("create_object_successful_ms", request_id.c_str(),
           time_in_millisecond);
  s3_stats_timing("multipart_create_object_success", time_in_millisecond);
  // mark rollback point
  add_task_rollback(
      std::bind(&S3PostMultipartObjectAction::rollback_create, this));
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  create_object_timer.stop();
  LOG_PERF("create_object_failed_ms", request_id.c_str(),
           create_object_timer.elapsed_time_in_millisec());
  s3_stats_timing("multipart_create_object_failed",
                  create_object_timer.elapsed_time_in_millisec());

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }

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

void S3PostMultipartObjectAction::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  probable_oid_list.clear();
  // for now, no need to track of old oid
  // add new oid to probable object oid list
  probable_oid_list[S3M0Uint128Helper::to_string(oid)] =
      object_multipart_metadata->create_probable_delete_record(layout_id);
  if (!probable_oid_list.empty()) {
    if (!clovis_kv_writer) {
      clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
          request, s3_clovis_api);
    }
    clovis_kv_writer->put_keyval(
        global_probable_dead_object_list_index_oid, probable_oid_list,
        std::bind(&S3PostMultipartObjectAction::next, this),
        std::bind(&S3PostMultipartObjectAction::
                       add_object_oid_to_probable_dead_oid_list_failed,
                  this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void
S3PostMultipartObjectAction::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Trigger rollback to undo changes done and report error
  rollback_start();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
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
    probable_oid_list.erase(S3M0Uint128Helper::to_string(oid));
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

  object_multipart_metadata->set_layout_id(layout_id);

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      object_multipart_metadata->add_user_defined_attribute(it.first,
                                                            it.second);
    }
  }

  object_multipart_metadata->set_tags(new_object_tags_map);
  // to rest Date and Last-Modfied time object metadata
  object_multipart_metadata->reset_date_time_to_current();
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

  part_metadata =
      part_metadata_factory->create_part_metadata_obj(request, upload_id, 0);
  part_metadata->create_index(
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index_successful,
                this),
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::create_part_meta_index_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // mark rollback point
  add_task_rollback(std::bind(
      &S3PostMultipartObjectAction::rollback_create_part_meta_index, this));
  next();
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

    if (S3Option::get_instance()->is_getoid_enabled()) {
      request->set_out_header_value(
          "OID", S3M0Uint128Helper::to_string(object_metadata->get_oid()));
      request->set_out_header_value(
          "layout-id", std::to_string(object_metadata->get_layout_id()));
    }

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
  cleanup_oid_from_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PostMultipartObjectAction::cleanup_oid_from_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::vector<std::string> clean_valid_oid_from_probable_dead_object_list_index;

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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
void S3PostMultipartObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  "");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

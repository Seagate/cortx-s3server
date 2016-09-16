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

#include "s3_delete_bucket_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_uri_to_mero_oid.h"

S3DeleteBucketAction::S3DeleteBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), is_bucket_empty(false), delete_successful(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  multipart_present = false;
  setup_steps();
}

void S3DeleteBucketAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind( &S3DeleteBucketAction::fetch_bucket_metadata, this ));
  add_task(std::bind( &S3DeleteBucketAction::fetch_first_object_metadata, this ));
  add_task(std::bind( &S3DeleteBucketAction::fetch_multipart_objects, this ));
  add_task(std::bind( &S3DeleteBucketAction::remove_part_indexes, this ));
  add_task(std::bind( &S3DeleteBucketAction::remove_multipart_index, this ));
  add_task(std::bind( &S3DeleteBucketAction::delete_bucket, this ));
  add_task(std::bind( &S3DeleteBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3DeleteBucketAction::fetch_bucket_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  // Trigger metadata read async operation with callback
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3DeleteBucketAction::next, this), std::bind( &S3DeleteBucketAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_first_object_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    clovis_kv_reader =
        std::make_shared<S3ClovisKVSReader>(request, s3_clovis_api);
    // Try to fetch one object at least
    struct m0_uint128 oid = bucket_metadata->get_object_list_index_oid();
    // If no object list index oid then it means bucket is empty
    if (oid.u_lo == 0ULL && oid.u_hi == 0ULL) {
      is_bucket_empty = true;
      next();
    } else {
      clovis_kv_reader->next_keyval(
          bucket_metadata->get_object_list_index_oid(), last_key, 1,
          std::bind(
              &S3DeleteBucketAction::fetch_first_object_metadata_successful,
              this),
          std::bind(&S3DeleteBucketAction::fetch_first_object_metadata_failed,
                    this));
    }
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_first_object_metadata_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  is_bucket_empty = false;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_first_object_metadata_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "There is no object in bucket\n");
    is_bucket_empty = true;
    next();
  } else {
    is_bucket_empty = false;
    s3_log(S3_LOG_ERROR, "Failed to retrieve object metadata\n");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_multipart_objects() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  struct m0_uint128  empty_indx_oid = {0ULL, 0ULL};
  struct m0_uint128 indx_oid = bucket_metadata->get_multipart_index_oid();
  //If the index oid is 0 then it implies there is no multipart metadata
  if (m0_uint128_cmp(&indx_oid, &empty_indx_oid) != 0) {
    multipart_present = true;
    //There is an oid for index present, so read objects from it
    size_t count = S3Option::get_instance()->get_clovis_idx_fetch_count();
    clovis_kv_reader =
        std::make_shared<S3ClovisKVSReader>(request, s3_clovis_api);
    clovis_kv_reader->next_keyval(indx_oid, last_key, count, std::bind( &S3DeleteBucketAction::fetch_multipart_objects_successful, this), std::bind( &S3DeleteBucketAction::next, this));
  } else {
    s3_log(S3_LOG_DEBUG, "Multipart index not present\n");
    next();
  }
}

void S3DeleteBucketAction::fetch_multipart_objects_successful() {
  s3_log(S3_LOG_DEBUG, "Found multipart uploads listing\n");
  size_t return_list_size = 0;
  auto& kvps = clovis_kv_reader->get_key_values();
  size_t count_we_requested = S3Option::get_instance()->get_clovis_idx_fetch_count();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    s3_log(S3_LOG_DEBUG, "Parsing Multipart object metadata = %s\n", kv.first.c_str());
    auto object = std::make_shared<S3ObjectMetadata>(request, true);

    object->from_json(kv.second);
    multipart_objects[kv.first] = object->get_upload_id();
    part_oids.push_back(object->get_part_index_oid());
    return_list_size++;

    if (--length == 0 || return_list_size == count_we_requested) {
      // this is the last element returned or we reached limit requested
      last_key = kv.first;
      break;
    }
  }
  if (kvps.size() < count_we_requested) {
    next();
  } else {
    fetch_multipart_objects();
  }
}

void S3DeleteBucketAction::remove_part_indexes() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (part_oids.size() != 0) {
    clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
    clovis_kv_writer->delete_indexes(
        part_oids,
        std::bind(&S3DeleteBucketAction::remove_part_indexes_successful, this),
        std::bind(&S3DeleteBucketAction::remove_part_indexes_failed, this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::remove_part_indexes_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  int i;
  bool partial_failure = false;
  for (multipart_kv = multipart_objects.begin(), i = 0; multipart_kv != multipart_objects.end(); multipart_kv++, i++) {
    if (clovis_kv_writer->get_op_ret_code_for(i) != 0 &&
        clovis_kv_writer->get_op_ret_code_for(i) != -ENOENT) {
      partial_failure = true;
    }
  }
  if (partial_failure) {
    s3_log(S3_LOG_WARN, "Failed to delete some of the multipart part metadata\n");
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  next();
}

void S3DeleteBucketAction::remove_part_indexes_failed() {
  s3_log(S3_LOG_WARN, "Failed to delete multipart part metadata\n");
  next();
}

void S3DeleteBucketAction::remove_multipart_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (multipart_present) {
    clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
    clovis_kv_writer->delete_index(bucket_metadata->get_multipart_index_oid(), std::bind( &S3DeleteBucketAction::next, this), std::bind( &S3DeleteBucketAction::next, this));
  } else {
   next();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::delete_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata->remove(std::bind( &S3DeleteBucketAction::delete_bucket_successful, this), std::bind( &S3DeleteBucketAction::delete_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::delete_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  delete_successful = true;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::delete_bucket_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Bucket deletion failed\n");
  delete_successful = false;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (is_bucket_empty == false) {
    // Bucket not empty, cannot delete
    S3Error error("BucketNotEmpty", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (delete_successful) {
    request->send_response(S3HttpSuccess204);
  } else {
    S3Error error("InternalError", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

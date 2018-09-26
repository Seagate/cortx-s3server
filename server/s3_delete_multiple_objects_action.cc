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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 3-Feb-2016
 */

#include "s3_delete_multiple_objects_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_option.h"
#include "s3_perf_logger.h"

// AWS allows to delete maximum 1000 objects in one call
#define MAX_OBJS_ALLOWED_TO_DELETE 1000

S3DeleteMultipleObjectsAction::S3DeleteMultipleObjectsAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_md_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_md_factory,
    std::shared_ptr<S3ClovisWriterFactory> writer_factory,
    std::shared_ptr<S3ClovisKVSReaderFactory> kvs_reader_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> kvs_writer_factory)
    : S3Action(req, false),
      delete_index_in_req(0),
      at_least_one_delete_successful(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: Delete Multiple Objects API. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  object_list_index_oid = {0ULL, 0ULL};

  if (bucket_md_factory) {
    bucket_metadata_factory = bucket_md_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  if (object_md_factory) {
    object_metadata_factory = object_md_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }

  if (writer_factory) {
    clovis_writer_factory = writer_factory;
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }

  if (kvs_reader_factory) {
    clovis_kvs_reader_factory = kvs_reader_factory;
  } else {
    clovis_kvs_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  if (kvs_writer_factory) {
    clovis_kvs_writer_factory = kvs_writer_factory;
  } else {
    clovis_kvs_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  std::shared_ptr<ClovisAPI> s3_clovis_api =
      std::make_shared<ConcreteClovisAPI>();

  clovis_kv_reader = clovis_kvs_reader_factory->create_clovis_kvs_reader(
      request, s3_clovis_api);

  clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
      request, s3_clovis_api);

  clovis_writer = clovis_writer_factory->create_clovis_writer(request);

  setup_steps();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3DeleteMultipleObjectsAction::validate_request, this));
  add_task(std::bind(&S3DeleteMultipleObjectsAction::fetch_bucket_info, this));
  add_task(std::bind(&S3DeleteMultipleObjectsAction::fetch_objects_info, this));
  // Delete will be cycling between delete_objects_metadata and delete_objects
  add_task(std::bind(&S3DeleteMultipleObjectsAction::send_response_to_s3_client,
                     this));
  // ...
}

void S3DeleteMultipleObjectsAction::validate_request() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (request->has_all_body_content()) {
    validate_request_body(request->get_full_body_content_as_string());
  } else {
    request->listen_for_incoming_data(
        std::bind(&S3DeleteMultipleObjectsAction::consume_incoming_content,
                  this),
        request->get_data_length() /* we ask for all */
        );
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (request->has_all_body_content()) {
    validate_request_body(request->get_full_body_content_as_string());
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::validate_request_body(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  MD5hash calc_md5;
  calc_md5.Update(content.c_str(), content.length());
  calc_md5.Finalize();

  std::string req_md5_str = request->get_header_value("content-md5");
  std::string calc_md5_str = calc_md5.get_md5_base64enc_string();
  if (calc_md5_str != req_md5_str) {
    // Request payload was corrupted in transit.
    set_s3_error("BadDigest");
    send_response_to_s3_client();
  } else {
    delete_request.initialize(content);
    if (delete_request.isOK()) {
      // AWS allows to delete maximum 1000 objects in one call
      if (delete_request.get_count() > MAX_OBJS_ALLOWED_TO_DELETE) {
        set_s3_error("MaxMessageLengthExceeded");
        send_response_to_s3_client();
      } else {
        next();
      }
    } else {
      invalid_request = true;
      set_s3_error("MalformedXML");
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (s3_fi_is_enabled("fail_fetch_bucket_info")) {
    s3_fi_enable_once("clovis_kv_get_fail");
  }

  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(
      std::bind(&S3DeleteMultipleObjectsAction::next, this),
      std::bind(&S3DeleteMultipleObjectsAction::fetch_bucket_info_failed,
                this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Fetching of bucket information failed\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void S3DeleteMultipleObjectsAction::fetch_objects_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  object_list_index_oid = bucket_metadata->get_object_list_index_oid();
  if (object_list_index_oid.u_lo == 0ULL &&
      object_list_index_oid.u_hi == 0ULL) {
    for (auto& key : keys_to_delete) {
      delete_objects_response.add_success(key);
    }
    send_response_to_s3_client();
  } else {
    if (delete_index_in_req < delete_request.get_count()) {
      keys_to_delete.clear();
      keys_to_delete = delete_request.get_keys(
          delete_index_in_req,
          S3Option::get_instance()->get_clovis_idx_fetch_count());
      if (s3_fi_is_enabled("fail_fetch_objects_info")) {
        s3_fi_enable_once("clovis_kv_get_fail");
      }
      clovis_kv_reader->get_keyval(
          object_list_index_oid, keys_to_delete,
          std::bind(
              &S3DeleteMultipleObjectsAction::fetch_objects_info_successful,
              this),
          std::bind(&S3DeleteMultipleObjectsAction::fetch_objects_info_failed,
                    this));
      delete_index_in_req += keys_to_delete.size();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::fetch_objects_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    for (auto& key : keys_to_delete) {
      delete_objects_response.add_success(key);
    }
    if (delete_index_in_req < delete_request.get_count()) {
      fetch_objects_info();
    } else {
      send_response_to_s3_client();
    }
  } else {
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::fetch_objects_info_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  // Create a list of objects found to be deleted

  auto& kvps = clovis_kv_reader->get_key_values();
  objects_metadata.clear();
  oids_to_delete.clear();

  bool atleast_one_json_error = false;
  bool all_had_json_error = true;
  for (auto& kv : kvps) {
    if ((kv.second.first == 0) && (!kv.second.second.empty())) {
      s3_log(S3_LOG_DEBUG, request_id, "Found Object metadata for = %s\n",
             kv.first.c_str());
      auto object =
          object_metadata_factory->create_object_metadata_obj(request);
      if (object->from_json(kv.second.second) != 0) {
        atleast_one_json_error = true;
        s3_log(S3_LOG_ERROR, request_id,
               "Json Parsing failed. Index oid = "
               "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
               object_list_index_oid.u_hi, object_list_index_oid.u_lo,
               kv.first.c_str(), kv.second.second.c_str());
        delete_objects_response.add_failure(kv.first, "InternalError");
        object->mark_invalid();
      } else {
        all_had_json_error = false;  // at least one good object to delete
        objects_metadata.push_back(object);
        oids_to_delete.push_back(object->get_oid());
        layout_id_for_objs_to_delete.push_back(object->get_layout_id());
      }
    } else {
      s3_log(S3_LOG_DEBUG, request_id, "Object metadata missing for = %s\n",
             kv.first.c_str());
      delete_objects_response.add_success(kv.first);
    }
  }
  if (atleast_one_json_error) {
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
  }
  if (all_had_json_error) {
    // Fetch more of present, else just respond
    if (delete_index_in_req < delete_request.get_count()) {
      // Try to delete the remaining
      fetch_objects_info();
    } else {
      if (!at_least_one_delete_successful) {
        set_s3_error("InternalError");
      }
      send_response_to_s3_client();
    }
  } else {
    delete_objects_metadata();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::delete_objects_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  std::vector<std::string> keys;
  for (auto& obj : objects_metadata) {
    if (obj->get_state() != S3ObjectMetadataState::invalid) {
      keys.push_back(obj->get_object_name());
    }
  }
  if (s3_fi_is_enabled("fail_delete_objects_metadata")) {
    s3_fi_enable_once("clovis_kv_delete_fail");
  }
  clovis_kv_writer->delete_keyval(
      object_list_index_oid, keys,
      std::bind(
          &S3DeleteMultipleObjectsAction::delete_objects_metadata_successful,
          this),
      std::bind(&S3DeleteMultipleObjectsAction::delete_objects_metadata_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::delete_objects_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  at_least_one_delete_successful = true;
  for (auto& obj : objects_metadata) {
    delete_objects_response.add_success(obj->get_object_name());
  }
  delete_objects();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::delete_objects_metadata_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  uint obj_index = 0;
  for (auto& obj : objects_metadata) {
    if (clovis_kv_writer->get_op_ret_code_for_del_kv(obj_index) == -ENOENT) {
      at_least_one_delete_successful = true;
      delete_objects_response.add_success(obj->get_object_name());
    } else {
      delete_objects_response.add_failure(obj->get_object_name(),
                                          "InternalError");
    }
    ++obj_index;
  }
  if (delete_index_in_req < delete_request.get_count()) {
    // Try to delete the remaining
    fetch_objects_info();
  } else {
    if (!at_least_one_delete_successful) {
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::delete_objects() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (oids_to_delete.empty()) {
    if (delete_index_in_req < delete_request.get_count()) {
      // Try to delete the remaining
      fetch_objects_info();
    } else {
      send_response_to_s3_client();
    }
  } else {
    // Now trigger the delete.
    clovis_writer->delete_objects(
        oids_to_delete, layout_id_for_objs_to_delete,
        std::bind(&S3DeleteMultipleObjectsAction::delete_objects_successful,
                  this),
        std::bind(&S3DeleteMultipleObjectsAction::delete_objects_failed, this));
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::delete_objects_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (delete_index_in_req < delete_request.get_count()) {
    // Try to delete the remaining
    fetch_objects_info();
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::delete_objects_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  uint obj_index = 0;
  bool delete_obj_failed = false;
  if (clovis_writer->get_state() == S3ClovisWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
    s3_log(S3_LOG_ERROR, request_id,
           "delete_objects_failed called due to clovis_entity_open failure\n");
    send_response_to_s3_client();
  } else {
    for (auto& obj : objects_metadata) {
      if (clovis_writer->get_op_ret_code_for_delete_op(obj_index) == -ENOENT) {
        // do nothing here
      } else {
        s3_log(S3_LOG_ERROR, request_id,
               "Deletion of object with oid "
               "%" SCNx64 " : %" SCNx64 " failed\n",
               obj->get_oid().u_hi, obj->get_oid().u_lo);
        delete_obj_failed = true;
      }
      ++obj_index;
    }
    if (delete_obj_failed) {
      s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
             S3_IEM_DELETE_OBJ_FAIL_JSON);
    }
    if (delete_index_in_req < delete_request.get_count()) {
      // Try to delete the remaining
      fetch_objects_info();
    } else {
      send_response_to_s3_client();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteMultipleObjectsAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

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
    std::string& response_xml = delete_objects_response.to_xml();

    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, request_id, "Object list response_xml = %s\n",
           response_xml.c_str());

    request->send_response(S3HttpSuccess200, response_xml);
  }
  request->resume();

  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}

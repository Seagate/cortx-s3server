/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Siddhivinayak Shanbhag <siddhivinayak.shanbhag@seagate.com>
 * Original creation date: 24-January-2019
 */

#include "s3_put_object_tagging_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3PutObjectTaggingAction::S3PutObjectTaggingAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3PutTagsBodyFactory> object_body_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: PutObjectTaggingAction. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

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
  if (object_body_factory) {
    put_object_tag_body_factory = object_body_factory;
  } else {
    put_object_tag_body_factory = std::make_shared<S3PutTagsBodyFactory>();
  }
  object_list_index_oid = {0ULL, 0ULL};
  setup_steps();
}

void S3PutObjectTaggingAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3PutObjectTaggingAction::validate_request, this));
  add_task(
      std::bind(&S3PutObjectTaggingAction::validate_request_xml_tags, this));
  add_task(std::bind(&S3PutObjectTaggingAction::fetch_bucket_info, this));
  add_task(std::bind(&S3PutObjectTaggingAction::get_object_metadata, this));
  add_task(
      std::bind(&S3PutObjectTaggingAction::save_tags_to_object_metadata, this));
  add_task(
      std::bind(&S3PutObjectTaggingAction::send_response_to_s3_client, this));
}

void S3PutObjectTaggingAction::validate_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (request->has_all_body_content()) {
    new_object_tags = request->get_full_body_content_as_string();
    validate_request_body(new_object_tags);
  } else {
    // Start streaming, logically pausing action till we get data.
    request->listen_for_incoming_data(
        std::bind(&S3PutObjectTaggingAction::consume_incoming_content, this),
        request->get_data_length() /* we ask for all */
        );
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectTaggingAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, request_id, "Consume data\n");
  if (request->has_all_body_content()) {
    new_object_tags = request->get_full_body_content_as_string();
    validate_request_body(new_object_tags);
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
}

void S3PutObjectTaggingAction::validate_request_body(std::string content) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  put_object_tag_body =
      put_object_tag_body_factory->create_put_resource_tags_body(content,
                                                                 request_id);
  if (put_object_tag_body->isOK()) {
    object_tags_map = put_object_tag_body->get_resource_tags_as_map();
    next();
  } else {
    set_s3_error("MalformedXML");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectTaggingAction::validate_request_xml_tags() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (put_object_tag_body->validate_object_xml_tags(object_tags_map)) {
    next();
  } else {
    set_s3_error("InvalidTagError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectTaggingAction::fetch_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(
      std::bind(&S3PutObjectTaggingAction::next, this),
      std::bind(&S3PutObjectTaggingAction::fetch_bucket_info_failed, this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_tagging_action_fetch_bucket_info_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectTaggingAction::fetch_bucket_info_failed() {
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

void S3PutObjectTaggingAction::get_object_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Fetching object metadata\n");
  object_list_index_oid = bucket_metadata->get_object_list_index_oid();
  if (object_list_index_oid.u_lo == 0ULL &&
      object_list_index_oid.u_hi == 0ULL) {
    set_s3_error("NoSuchKey");
    send_response_to_s3_client();
  } else {
    object_metadata = object_metadata_factory->create_object_metadata_obj(
        request, object_list_index_oid);
    object_metadata->load(
        std::bind(&S3PutObjectTaggingAction::next, this),
        std::bind(&S3PutObjectTaggingAction::get_object_metadata_failed, this));
  }
}

void S3PutObjectTaggingAction::get_object_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    set_s3_error("NoSuchKey");
  } else if (object_metadata->get_state() ==
             S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Object metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectTaggingAction::save_tags_to_object_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  object_metadata->set_tags(object_tags_map);
  object_metadata->save_metadata(
      std::bind(&S3PutObjectTaggingAction::next, this),
      std::bind(&S3PutObjectTaggingAction::save_tags_to_object_metadata_failed,
                this));
}

void S3PutObjectTaggingAction::save_tags_to_object_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Object metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectTaggingAction::send_response_to_s3_client() {
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
  } else {
    request->send_response(S3HttpSuccess200);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  done();
  i_am_done();  // self delete
}

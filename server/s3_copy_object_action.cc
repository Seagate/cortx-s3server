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
#include "s3_copy_object_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_motr_layout.h"
#include "s3_m0_uint128_helper.h"
#include "s3_probable_delete_record.h"
#include "s3_uri_to_motr_oid.h"

S3CopyObjectAction::S3CopyObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motrwriter_s3_factory,
    std::shared_ptr<S3MotrReaderFactory> motrreader_s3_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3PutObjectActionBase(std::move(req), std::move(bucket_meta_factory),
                            std::move(object_meta_factory), std::move(motr_api),
                            std::move(motrwriter_s3_factory),
                            std::move(kv_writer_factory)) {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: CopyObject. Destination: [%s], Source: [%s]\n",
         request->get_object_uri().c_str(),
         request->get_headers_copysource().c_str());

  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);

  if (motrreader_s3_factory) {
    motr_reader_factory = std::move(motrreader_s3_factory);
  } else {
    motr_reader_factory = std::make_shared<S3MotrReaderFactory>();
  }
  setup_steps();
};

void S3CopyObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3CopyObjectAction::validate_copyobject_request, this);
  ACTION_TASK_ADD(S3CopyObjectAction::set_source_bucket_authorization_metadata,
                  this);
  ACTION_TASK_ADD(S3CopyObjectAction::check_source_bucket_authorization, this);
  ACTION_TASK_ADD(S3CopyObjectAction::create_object, this);
  ACTION_TASK_ADD(S3CopyObjectAction::copy_object, this);
  ACTION_TASK_ADD(S3CopyObjectAction::save_metadata, this);
  ACTION_TASK_ADD(S3CopyObjectAction::send_response_to_s3_client, this);
}

// Validate source bucket and object
void S3CopyObjectAction::validate_copyobject_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  auto meta_directive_hdr =
      request->get_header_value("x-amz-metadata-directive");
  auto tagging_directive_hdr =
      request->get_header_value("x-amz-tagging-directive");

  if (!meta_directive_hdr.empty()) {
    s3_log(S3_LOG_DEBUG, stripped_request_id,
           "Received x-amz-metadata-directive header, value: %s",
           meta_directive_hdr.c_str());
    if (meta_directive_hdr == DirectiveValueReplace) {
      s3_put_action_state = S3PutObjectActionState::validationFailed;
      set_s3_error("NotImplemented");
      send_response_to_s3_client();
      return;
    } else if (meta_directive_hdr != DirectiveValueCOPY) {
      s3_put_action_state = S3PutObjectActionState::validationFailed;
      set_s3_error("InvalidArgument");
      send_response_to_s3_client();
      return;
    }
  }
  if (!tagging_directive_hdr.empty()) {
    s3_log(S3_LOG_DEBUG, stripped_request_id,
           "Received x-amz-tagging-directive header, value: %s",
           tagging_directive_hdr.c_str());
    if (tagging_directive_hdr == DirectiveValueReplace) {
      s3_put_action_state = S3PutObjectActionState::validationFailed;
      set_s3_error("NotImplemented");
      send_response_to_s3_client();
      return;
    } else if (tagging_directive_hdr != DirectiveValueCOPY) {
      s3_put_action_state = S3PutObjectActionState::validationFailed;
      set_s3_error("InvalidArgument");
      send_response_to_s3_client();
      return;
    }
  }
  if (additional_bucket_name.empty() || additional_object_name.empty()) {
    set_s3_error("InvalidArgument");
    send_response_to_s3_client();
    return;
  } else if (if_source_and_destination_same()) {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidRequest");
    send_response_to_s3_client();
    return;
  }
  if (MaxCopyObjectSourceSize <
      additional_object_metadata->get_content_length()) {
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("InvalidRequest");
    send_response_to_s3_client();
  } else {
    total_data_to_stream = additional_object_metadata->get_content_length();
    next();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

const char xml_spaces[] = "        ";
// Shall be 8 bytes (size of cipher block)

bool S3CopyObjectAction::copy_object_cb() {
  if (check_shutdown_and_rollback() || !request->client_connected()) {
    return true;
  }
  if (response_started) {
    request->send_reply_body(xml_spaces, sizeof(xml_spaces) - 1);
  }
  return false;
}

// Copy source object to destination object
void S3CopyObjectAction::copy_object() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (!total_data_to_stream) {
    s3_log(S3_LOG_DEBUG, stripped_request_id, "Source object is empty");
    next();
    return;
  }
  bool f_success = false;
  try {
    object_data_copier.reset(new S3ObjectDataCopier(
        request, motr_writer, motr_reader_factory, s3_motr_api));

    object_data_copier->copy(
        additional_object_metadata->get_oid(), total_data_to_stream,
        additional_object_metadata->get_layout_id(),
        additional_object_metadata->get_pvid(),
        std::bind(&S3CopyObjectAction::copy_object_cb, this),
        std::bind(&S3CopyObjectAction::copy_object_success, this),
        std::bind(&S3CopyObjectAction::copy_object_failed, this));
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

  } else if (total_data_to_stream > MINIMUM_ALLOWED_PART_SIZE) {

    start_response();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3CopyObjectAction::copy_object_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::writeComplete;
  object_data_copier.reset();
  next();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3CopyObjectAction::copy_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::writeFailed;
  set_s3_error(object_data_copier->get_s3_error());

  object_data_copier.reset();
  send_response_to_s3_client();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3CopyObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("put_object_action_save_metadata_pass");

  new_object_metadata->reset_date_time_to_current();
  new_object_metadata->set_content_length(std::to_string(total_data_to_stream));
  new_object_metadata->set_content_type(
      additional_object_metadata->get_content_type());
  new_object_metadata->set_md5(motr_writer->get_content_md5());
  new_object_metadata->setacl(auth_acl);

  // put source object tags on new object
  s3_log(S3_LOG_DEBUG, stripped_request_id,
         "copying source object tags on new object..");
  new_object_metadata->set_tags(additional_object_metadata->get_tags());

  // copy source object user-defined metadata, to new object
  s3_log(S3_LOG_DEBUG, stripped_request_id,
         "copying source object metadata on new object..");
  for (auto it : additional_object_metadata->get_user_attributes()) {
    new_object_metadata->add_user_defined_attribute(it.first, it.second);
  }

  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  new_object_metadata->save(
      std::bind(&S3CopyObjectAction::save_object_metadata_success, this),
      std::bind(&S3CopyObjectAction::save_object_metadata_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3CopyObjectAction::save_object_metadata_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_put_action_state = S3PutObjectActionState::metadataSaved;
  next();
}

void S3CopyObjectAction::save_object_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::metadataSaveFailed;
  if (new_object_metadata->get_state() ==
      S3ObjectMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "failed to save object metadata.");
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

const char xml_decl[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
const char xml_comment_begin[] = "<!--   \n";
const char xml_comment_end[] = "\n   -->\n";

std::string S3CopyObjectAction::get_response_xml() {
  std::string response_xml;

  if (!response_started) {
    response_xml += xml_decl;
  }
  response_xml +=
      "<CopyObjectResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
  response_xml += S3CommonUtilities::format_xml_string(
      "LastModified", new_object_metadata->get_last_modified_iso());
  // ETag for the destination object would be same as Etag for Source Object
  response_xml += S3CommonUtilities::format_xml_string(
      "ETag", new_object_metadata->get_md5(), true);
  response_xml += "\n</CopyObjectResult>";
  return response_xml;
}

void S3CopyObjectAction::start_response() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  request->set_out_header_value("Content-Type", "application/xml");
  request->set_out_header_value("Connection", "close");

  request->send_reply_start(S3HttpSuccess200);
  request->send_reply_body(xml_decl, sizeof(xml_decl) - 1);
  request->send_reply_body(xml_comment_begin, sizeof(xml_comment_begin) - 1);

  response_started = true;
}

void S3CopyObjectAction::send_response_to_s3_client() {
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
    assert(s3_put_action_state != S3PutObjectActionState::metadataSaved);

    S3Error error(get_s3_error_code(), request->get_request_id());

    if (S3PutObjectActionState::validationFailed == s3_put_action_state &&
        "InvalidRequest" == get_s3_error_code()) {
      if (if_source_and_destination_same()) {  // Source and Destination same
        error.set_auth_error_message(InvalidRequestSourceAndDestinationSame);
      } else if (additional_object_metadata->get_content_length() >
                 MaxCopyObjectSourceSize) {  // Source object size greater than
                                             // 5GB
        error.set_auth_error_message(
            InvalidRequestSourceObjectSizeGreaterThan5GB);
      }
    }
    response_xml = std::move(error.to_xml(response_started));
    http_status_code = error.get_http_status_code();

    if (!response_started && (get_s3_error_code() == "ServiceUnavailable" ||
                              get_s3_error_code() == "InternalError")) {
      request->set_out_header_value("Connection", "close");
    }
    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }
  } else {
    assert(s3_put_action_state == S3PutObjectActionState::metadataSaved);
    s3_put_action_state = S3PutObjectActionState::completed;

    response_xml = get_response_xml();
    http_status_code = S3HttpSuccess200;
  }
  if (response_started) {
    request->send_reply_body(xml_comment_end, sizeof(xml_comment_end) - 1);
    request->send_reply_body(response_xml.c_str(), response_xml.length());
    request->send_reply_end();
    request->close_connection();
  } else {
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(http_status_code, std::move(response_xml));
  }
#ifndef S3_GOOGLE_TEST
  startcleanup();
#endif  // S3_GOOGLE_TEST
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

bool S3CopyObjectAction::if_source_and_destination_same() {
  return ((additional_bucket_name == request->get_bucket_name()) &&
          (additional_object_name == request->get_object_name()));
}

void S3CopyObjectAction::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  request->set_action_str("PutObject");
  request->reset_action_list();

  if (!additional_object_metadata->get_tags().empty()) {
    request->set_action_list("PutObjectTagging");
  }
  // request->set_action_list("PutObjectAcl");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::set_source_bucket_authorization_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  auth_acl = request->get_default_acl();
  auth_client->set_get_method = true;

  auth_client->set_entity_path("/" + additional_bucket_name + "/" +
                               additional_object_name);

  request->reset_action_list();
  auth_client->set_acl_and_policy(
      additional_object_metadata->get_encoded_object_acl(),
      additional_bucket_metadata->get_policy_as_json());
  request->set_action_str("GetObject");
  if (!additional_object_metadata->get_tags().empty()) {
    request->set_action_list("GetObjectTagging");
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::check_source_bucket_authorization() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  auth_client->check_authorization(
      std::bind(&S3CopyObjectAction::check_source_bucket_authorization_success,
                this),
      std::bind(&S3CopyObjectAction::check_source_bucket_authorization_failed,
                this));
}

void S3CopyObjectAction::check_source_bucket_authorization_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // Do destination bucket authorization for secondary operations
  request->set_action_str("PutObject");

  // clear action list
  request->reset_action_list();

  auth_client->set_entity_path("/" + request->get_bucket_name() + "/" +
                               request->get_object_name());

  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  if (!source_object_metadata->get_tags().empty()) {
    request->set_action_list("PutObjectTagging");
  }
  // request->set_action_list("PutObjectAcl");
  if (!request->get_action_list().empty()) {
    auth_client->check_authorization(
        std::bind(&S3CopyObjectAction::next, this),
        std::bind(
            &S3CopyObjectAction::check_destination_bucket_authorization_failed,
            this));
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3CopyObjectAction::check_destination_bucket_authorization_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::validationFailed;
  std::string error_code = auth_client->get_error_code();

  set_s3_error(error_code);
  s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
         error_code.c_str());

  if (request->client_connected()) {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3CopyObjectAction::check_source_bucket_authorization_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_put_action_state = S3PutObjectActionState::validationFailed;
  std::string error_code = auth_client->get_error_code();

  set_s3_error(error_code);
  s3_log(S3_LOG_ERROR, request_id, "Authorization failure: %s\n",
         error_code.c_str());

  if (request->client_connected()) {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

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

#include "s3_copy_object_action.h"
#include "s3_log.h"
#include "s3_error_codes.h"
#include "s3_common_utilities.h"
#include "s3_motr_layout.h"
#include "s3_uri_to_motr_oid.h"

S3CopyObjectAction::S3CopyObjectAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3MotrWriterFactory> motrwriter_s3_factory,
    std::shared_ptr<S3MotrReaderFactory> motrreader_s3_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory)),
      write_in_progress(false),
      read_in_progress(false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  s3_log(S3_LOG_INFO, request_id,
         "S3 API: CopyObject. Destination: [%s], Source: [%s]\n",
         request->get_bucket_name().c_str(),
         request->get_copy_object_source().c_str());
  s3_copy_action_state = S3CopyObjectActionState::empty;

  old_object_oid = {0ULL, 0ULL};
  old_layout_id = -1;
  new_object_oid = {0ULL, 0ULL};

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  S3UriToMotrOID(s3_motr_api, request->get_object_uri().c_str(), request_id,
                 &new_object_oid);
  // Note valid value is set during create object
  layout_id = -1;
  tried_count = 0;
  salt = "uri_salt_";

  if (motrwriter_s3_factory) {
    motr_writer_factory = std::move(motrwriter_s3_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }
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
  ACTION_TASK_ADD(S3CopyObjectAction::create_object, this);
  ACTION_TASK_ADD(S3CopyObjectAction::read_object, this);
  ACTION_TASK_ADD(S3CopyObjectAction::initiate_data_streaming, this);
  ACTION_TASK_ADD(S3CopyObjectAction::save_metadata, this);
  ACTION_TASK_ADD(S3CopyObjectAction::send_response_to_s3_client, this);
}

void S3CopyObjectAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_copy_action_state = S3CopyObjectActionState::validationFailed;
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::fetch_object_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Found bucket metadata\n");
    object_list_oid = bucket_metadata->get_object_list_index_oid();
    if (object_list_oid.u_hi == 0ULL && object_list_oid.u_lo == 0ULL) {
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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::fetch_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  next();
}

void S3CopyObjectAction::validate_copyobject_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::read_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::initiate_data_streaming() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::save_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3CopyObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  std::string response_xml = get_response_xml();
  request->send_response(S3HttpSuccess200, response_xml);
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

std::string S3CopyObjectAction::get_response_xml() {
  std::string response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  response_xml +=
      "<CopyObjectResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
  response_xml += S3CommonUtilities::format_xml_string("LastModified", "");
  response_xml += S3CommonUtilities::format_xml_string("ETag", "");
  response_xml += "</CopyObjectResult>";
  return response_xml;
}

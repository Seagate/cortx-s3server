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

#include <evhttp.h>
#include <string>
#include <algorithm>

#include "s3_error_codes.h"
#include "s3_factory.h"
#include "s3_option.h"
#include "s3_common_utilities.h"
#include "motr_request_object.h"
#include "s3_stats.h"
#include "s3_audit_info_logger.h"

extern S3Option* g_option_instance;

MotrRequestObject::MotrRequestObject(
    evhtp_request_t* req, EvhtpInterface* evhtp_obj_ptr,
    std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buf_factory,
    EventInterface* event_obj_ptr)
    : RequestObject(req, evhtp_obj_ptr, async_buf_factory, event_obj_ptr),
      motr_api_type(MotrApiType::unsupported) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
}

MotrRequestObject::~MotrRequestObject() {
  s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");
}

void MotrRequestObject::set_key_name(const std::string& key) { key_name = key; }

const std::string& MotrRequestObject::get_key_name() { return key_name; }

void MotrRequestObject::set_object_oid_lo(const std::string& oid_lo) {
  object_oid_lo = oid_lo;
}

const std::string& MotrRequestObject::get_object_oid_lo() {
  return object_oid_lo;
}

void MotrRequestObject::set_object_oid_hi(const std::string& oid_hi) {
  object_oid_hi = oid_hi;
}

const std::string& MotrRequestObject::get_object_oid_hi() {
  return object_oid_hi;
}

void MotrRequestObject::set_index_id_lo(const std::string& index_lo) {
  index_id_lo = index_lo;
}

const std::string& MotrRequestObject::get_index_id_lo() { return index_id_lo; }

void MotrRequestObject::set_index_id_hi(const std::string& index_hi) {
  index_id_hi = index_hi;
}

const std::string& MotrRequestObject::get_index_id_hi() { return index_id_hi; }

void MotrRequestObject::set_api_type(MotrApiType api_type) {
  motr_api_type = api_type;
}

MotrApiType MotrRequestObject::get_api_type() { return motr_api_type; }

void MotrRequestObject::set_operation_code(MotrOperationCode operation_code) {
  motr_operation_code = operation_code;
}

MotrOperationCode MotrRequestObject::get_operation_code() {
  return motr_operation_code;
}

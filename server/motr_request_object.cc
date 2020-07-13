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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-JULY-2019
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
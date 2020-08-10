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

#pragma once

#ifndef __S3_SERVER_MOTR_REQUEST_OBJECT_H__
#define __S3_SERVER_MOTR_REQUEST_OBJECT_H__

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

/* libevhtp */
#include <gtest/gtest_prod.h>
#include "evhtp_wrapper.h"
#include "event_wrapper.h"

#include "request_object.h"
#include "s3_async_buffer_opt.h"
#include "s3_chunk_payload_parser.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_timer.h"
#include "s3_uuid.h"
#include "s3_audit_info.h"

class S3AsyncBufferOptContainerFactory;

class MotrRequestObject : public RequestObject {
 private:
  std::string object_oid_lo;
  std::string object_oid_hi;
  std::string index_id_lo;
  std::string index_id_hi;
  std::string key_name;

  MotrApiType motr_api_type;
  MotrOperationCode motr_operation_code;

 public:
  MotrRequestObject(
      evhtp_request_t* req, EvhtpInterface* evhtp_obj_ptr,
      std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buf_factory =
          nullptr,
      EventInterface* event_obj_ptr = nullptr);
  virtual ~MotrRequestObject();

  void set_api_type(MotrApiType apitype);
  virtual MotrApiType get_api_type();
  void set_operation_code(MotrOperationCode operation_code);
  virtual MotrOperationCode get_operation_code();

 public:
  virtual void set_key_name(const std::string& key);
  virtual const std::string& get_key_name();
  virtual void set_object_oid_lo(const std::string& oid_lo);
  virtual const std::string& get_object_oid_lo();
  virtual void set_object_oid_hi(const std::string& oid_hi);
  virtual const std::string& get_object_oid_hi();
  virtual void set_index_id_lo(const std::string& index_lo);
  virtual const std::string& get_index_id_lo();
  virtual void set_index_id_hi(const std::string& index_hi);
  virtual const std::string& get_index_id_hi();
};

#endif

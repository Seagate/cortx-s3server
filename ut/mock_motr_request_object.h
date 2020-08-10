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

#ifndef __S3_UT_MOCK_MOTR_REQUEST_OBJECT_H__
#define __S3_UT_MOCK_MOTR_REQUEST_OBJECT_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "motr_request_object.h"

class MockMotrRequestObject : public MotrRequestObject {
 public:
  MockMotrRequestObject(
      evhtp_request_t *req, EvhtpInterface *evhtp_obj_ptr,
      std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buf_factory =
          nullptr)
      : MotrRequestObject(req, evhtp_obj_ptr, async_buf_factory) {}
  MOCK_METHOD0(c_get_full_path, const char *());
  MOCK_METHOD0(c_get_full_encoded_path, const char *());
  MOCK_METHOD0(get_host_header, std::string());
  MOCK_METHOD0(get_data_length, size_t());
  MOCK_METHOD0(get_data_length_str, std::string());
  MOCK_METHOD0(get_full_body_content_as_string, std::string &());
  MOCK_METHOD0(http_verb, S3HttpVerb());
  MOCK_METHOD0(c_get_uri_query, const char *());
  MOCK_METHOD0(get_query_parameters,
               const std::map<std::string, std::string, compare> &());
  MOCK_METHOD0(get_request, evhtp_request_t *());
  MOCK_METHOD0(get_content_length, size_t());
  MOCK_METHOD0(get_content_length_str, std::string());
  MOCK_METHOD0(get_key_name, const std::string &());
  MOCK_METHOD0(get_object_oid_lo, const std::string &());
  MOCK_METHOD0(get_object_oid_hi, const std::string &());
  MOCK_METHOD0(get_index_id_lo, const std::string &());
  MOCK_METHOD0(get_index_id_hi, const std::string &());
  MOCK_METHOD1(set_key_name, void(const std::string &key));
  MOCK_METHOD1(set_object_oid_lo, void(const std::string &oid_lo));
  MOCK_METHOD1(set_object_oid_hi, void(const std::string &oid_hi));
  MOCK_METHOD1(set_index_id_lo, void(const std::string &index_lo));
  MOCK_METHOD1(set_index_id_hi, void(const std::string &index_hi));
  MOCK_CONST_METHOD0(has_all_body_content, bool());
  MOCK_METHOD0(is_chunked, bool());
  MOCK_METHOD0(pause, void());
  MOCK_METHOD1(resume, void(bool set_read_timeout_timer));
  MOCK_METHOD1(has_query_param_key, bool(std::string key));
  MOCK_METHOD1(set_api_type, void(MotrApiType));
  MOCK_METHOD1(get_query_string_value, std::string(std::string key));
  MOCK_METHOD0(get_api_type, MotrApiType());
  MOCK_METHOD1(respond_retry_after, void(int retry_after_in_secs));
  MOCK_METHOD2(set_out_header_value, void(std::string, std::string));
  MOCK_METHOD0(get_in_headers_copy, std::map<std::string, std::string> &());
  MOCK_METHOD2(send_response, void(int, std::string));
  MOCK_METHOD1(send_reply_start, void(int code));
  MOCK_METHOD2(send_reply_body, void(char *data, int length));
  MOCK_METHOD0(send_reply_end, void());
  MOCK_METHOD0(is_chunk_detail_ready, bool());
  MOCK_METHOD0(pop_chunk_detail, S3ChunkDetail());

  MOCK_METHOD2(listen_for_incoming_data,
               void(std::function<void()> callback, size_t notify_on_size));
  MOCK_METHOD1(get_header_value, std::string(std::string key));
};

#endif

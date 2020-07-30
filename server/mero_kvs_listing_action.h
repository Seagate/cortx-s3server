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

#ifndef __S3_SERVER_MERO_KVS_LIST_ACTION_H__
#define __S3_SERVER_MERO_KVS_LIST_ACTION_H__

#include <memory>

#include "mero_action_base.h"
#include "s3_clovis_kvs_reader.h"
#include "mero_kv_list_response.h"

class MeroKVSListingAction : public MeroAction {
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<ClovisAPI> mero_clovis_api;
  std::shared_ptr<S3ClovisKVSReaderFactory> mero_clovis_kvs_reader_factory;
  MeroKVListResponse kvs_response_list;
  m0_uint128 index_id;
  std::string last_key;  // last key during each iteration
  bool fetch_successful;

  // Request Input params
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  size_t max_keys;

 public:
  MeroKVSListingAction(
      std::shared_ptr<MeroRequestObject> req,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory =
          nullptr);
  void setup_steps();
  void validate_request();
  void get_next_key_value();
  void get_next_key_value_successful();
  void get_next_key_value_failed();

  void send_response_to_s3_client();
};

#endif

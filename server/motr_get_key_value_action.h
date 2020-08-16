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

#ifndef __MOTR_GET_KEY_VALUE_ACTION_H__
#define __MOTR_GET_KEY_VALUE_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_factory.h"
#include "motr_action_base.h"

class MotrGetKeyValueAction : public MotrAction {
  m0_uint128 index_id;
  std::shared_ptr<ClovisAPI> motr_clovis_api;
  std::shared_ptr<S3MotrKVSReader> clovis_kv_reader;

  std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory;

 public:
  MotrGetKeyValueAction(
      std::shared_ptr<MotrRequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr,
      std::shared_ptr<S3MotrKVSReaderFactory> clovis_motr_kvs_reader_factory =
          nullptr);

  void setup_steps();
  void fetch_key_value();
  void fetch_key_value_successful();
  void fetch_key_value_failed();
  void send_response_to_s3_client();
};
#endif

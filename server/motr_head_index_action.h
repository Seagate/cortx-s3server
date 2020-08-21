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

#ifndef __MOTR_HEAD_INDEX_ACTION_H__
#define __MOTR_HEAD_INDEX_ACTION_H__

#include <memory>
#include <gtest/gtest_prod.h>

#include "s3_factory.h"
#include "motr_action_base.h"

class MotrHeadIndexAction : public MotrAction {
  m0_uint128 index_id;
  std::shared_ptr<MotrAPI> motr_api;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory_ptr;

  void setup_steps();
  void validate_request();
  void check_index_exist();
  void check_index_exist_success();
  void check_index_exist_failure();
  void send_response_to_s3_client();

 public:
  MotrHeadIndexAction(
      std::shared_ptr<MotrRequestObject> req,
      std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory =
          nullptr);

  FRIEND_TEST(MotrHeadIndexActionTest, ValidIndexId);
  FRIEND_TEST(MotrHeadIndexActionTest, InvalidIndexId);
  FRIEND_TEST(MotrHeadIndexActionTest, EmptyIndexId);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExist);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistSuccess);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistFailureMissing);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistFailureInternalError);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistFailureFailedToLaunch);
  FRIEND_TEST(MotrHeadIndexActionTest, SendSuccessResponse);
  FRIEND_TEST(MotrHeadIndexActionTest, SendBadRequestResponse);
};
#endif

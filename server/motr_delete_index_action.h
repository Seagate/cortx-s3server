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

#ifndef __S3_SERVER_MOTR_INDEX_DELETE_ACTION_H__
#define __S3_SERVER_MOTR_INDEX_DELETE_ACTION_H__

#include <memory>
#include <gtest/gtest_prod.h>

#include "motr_action_base.h"
#include "s3_motr_kvs_writer.h"

class MotrDeleteIndexAction : public MotrAction {
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<MotrAPI> motr_api;
  std::shared_ptr<S3MotrKVSWriterFactory> motr_kvs_writer_factory_ptr;

  m0_uint128 index_id;

 public:
  MotrDeleteIndexAction(
      std::shared_ptr<MotrRequestObject> req,
      std::shared_ptr<S3MotrKVSWriterFactory> motr_kvs_reader_factory =
          nullptr);
  void setup_steps();

  void validate_request();
  void delete_index();
  void delete_index_successful();
  void delete_index_failed();

  void send_response_to_s3_client();

  FRIEND_TEST(MotrDeleteIndexActionTest, ConstructorTest);
  FRIEND_TEST(MotrDeleteIndexActionTest, ValidIndexId);
  FRIEND_TEST(MotrDeleteIndexActionTest, InvalidIndexId);
  FRIEND_TEST(MotrDeleteIndexActionTest, EmptyIndexId);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndex);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexSuccess);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexFailedIndexMissing);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexFailed);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexFailedToLaunch);
  FRIEND_TEST(MotrDeleteIndexActionTest, SendSuccessResponse);
  FRIEND_TEST(MotrDeleteIndexActionTest, SendBadRequestResponse);
};

#endif

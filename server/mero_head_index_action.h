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

#ifndef __MERO_HEAD_INDEX_ACTION_H__
#define __MERO_HEAD_INDEX_ACTION_H__

#include <memory>
#include <gtest/gtest_prod.h>

#include "s3_factory.h"
#include "mero_action_base.h"

class MeroHeadIndexAction : public MeroAction {
  m0_uint128 index_id;
  std::shared_ptr<ClovisAPI> mero_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;

  void setup_steps();
  void validate_request();
  void check_index_exist();
  void check_index_exist_success();
  void check_index_exist_failure();
  void send_response_to_s3_client();

 public:
  MeroHeadIndexAction(
      std::shared_ptr<MeroRequestObject> req,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_mero_kvs_reader_factory =
          nullptr);

  FRIEND_TEST(MeroHeadIndexActionTest, ValidIndexId);
  FRIEND_TEST(MeroHeadIndexActionTest, InvalidIndexId);
  FRIEND_TEST(MeroHeadIndexActionTest, EmptyIndexId);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExist);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistSuccess);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistFailureMissing);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistFailureInternalError);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistFailureFailedToLaunch);
  FRIEND_TEST(MeroHeadIndexActionTest, SendSuccessResponse);
  FRIEND_TEST(MeroHeadIndexActionTest, SendBadRequestResponse);
};
#endif

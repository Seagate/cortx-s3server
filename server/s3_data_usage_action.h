/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#include <map>
#include <string>

#include "s3_request_object.h"
#include "s3_action_base.h"
#include "s3_factory.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"

class S3DataUsageAction : public S3Action {
  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<S3MotrKVSReader> motr_kvs_reader;
  std::shared_ptr<MotrAPI> motr_kvs_api;

  std::string last_key;
  unsigned max_records_per_request;
  std::map<std::string, int64_t> counters;

  void get_data_usage_counters();
  void get_next_keyval_success();
  void get_next_keyval_failure();

 public:
  S3DataUsageAction(std::shared_ptr<S3RequestObject> req,
                    std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory =
                        nullptr,
                    std::shared_ptr<MotrAPI> motr_api = nullptr);
  virtual ~S3DataUsageAction();
  void send_response_to_s3_client();
  void setup_steps();
  std::string create_json_response();

  friend class S3DataUsageActionTest;
  FRIEND_TEST(S3DataUsageActionTest, Constructor);
  FRIEND_TEST(S3DataUsageActionTest, GetDataUsageCountersMissing);
  FRIEND_TEST(S3DataUsageActionTest, GetDataUsageCountersFailed);
  FRIEND_TEST(S3DataUsageActionTest, GetDataUsageCountersSuccess);
};

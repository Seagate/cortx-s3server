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

#ifndef __S3_UT_MOCK_S3_MOTR_KVS_WRITER_H__
#define __S3_UT_MOCK_S3_MOTR_KVS_WRITER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_motr_kvs_writer.h"

class MockS3MotrKVSWriter : public S3MotrKVSWriter {
 public:
  MockS3MotrKVSWriter(std::shared_ptr<RequestObject> req,
                      std::shared_ptr<MotrAPI> s3_motr_api)
      : S3MotrKVSWriter(req, s3_motr_api) {}
  MOCK_CONST_METHOD0(get_state, S3MotrKVSWriterOpState());
  MOCK_CONST_METHOD0(get_index_layout, struct s3_motr_idx_layout());
  MOCK_METHOD1(get_op_ret_code_for, int(int index));
  MOCK_METHOD1(get_op_ret_code_for_del_kv, int(int index));
  MOCK_METHOD3(create_index, void(const std::string& index_name,
                                  std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed));
  MOCK_METHOD3(create_index_with_oid,
               void(const struct m0_uint128&, std::function<void(void)>,
                    std::function<void(void)> on_failed));
  MOCK_METHOD3(delete_index,
               void(const struct s3_motr_idx_layout&, std::function<void(void)>,
                    std::function<void(void)> on_failed));
  MOCK_METHOD3(delete_indices,
               void(const std::vector<struct s3_motr_idx_layout>&,
                    std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD4(delete_keyval, void(const struct s3_motr_idx_layout&,
                                   const std::vector<std::string>&,
                                   std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed));

  MOCK_METHOD6(put_keyval,
               void(const struct s3_motr_idx_layout&, const std::string&,
                    const std::string&, std::function<void(void)> on_success,
                    std::function<void(void)> on_failed,
                    CallbackType callback));
  MOCK_METHOD5(put_keyval,
               void(const struct s3_motr_idx_layout&,
                    const std::map<std::string, std::string>& kv_list,
                    std::function<void(void)> on_success,
                    std::function<void(void)> on_failed,
                    CallbackType callback));
};
#endif

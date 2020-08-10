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

#ifndef __S3_UT_MOCK_S3_PROBABLE_DELETE_RECORD_H__
#define __S3_UT_MOCK_S3_PROBABLE_DELETE_RECORD_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_probable_delete_record.h"

using ::testing::_;
using ::testing::Return;

class MockS3ProbableDeleteRecord : public S3ProbableDeleteRecord {
 public:
  MockS3ProbableDeleteRecord(std::string rec_key, struct m0_uint128 old_oid,
                             std::string obj_key_in_index,
                             struct m0_uint128 new_oid, int layout_id,
                             struct m0_uint128 obj_list_idx_oid,
                             struct m0_uint128 objs_version_list_idx_oid,
                             std::string ver_key_in_index,
                             bool force_del = false, bool is_multipart = false,
                             struct m0_uint128 part_list_oid = {0ULL, 0ULL})
      : S3ProbableDeleteRecord(rec_key, old_oid, obj_key_in_index, new_oid,
                               layout_id, obj_list_idx_oid,
                               objs_version_list_idx_oid, ver_key_in_index,
                               force_del, is_multipart, part_list_oid) {}
  MOCK_METHOD0(get_state, S3ObjectMetadataState());
  MOCK_METHOD0(get_key, const std::string&());
  MOCK_METHOD0(get_current_object_oid, struct m0_uint128());
  MOCK_METHOD1(set_force_delete, void(bool));
  MOCK_METHOD0(to_json, std::string());
};

#endif

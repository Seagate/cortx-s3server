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

#ifndef __S3_SERVER_S3_PROBABLE_DELETE_RECORD_H__
#define __S3_SERVER_S3_PROBABLE_DELETE_RECORD_H__

#include <gtest/gtest_prod.h>
#include <string>

#include "s3_motr_rw_common.h"

// Record is stored as a value in global probable delete records list index and
// the "key" is
//    For new objects key is current object OID.
//    For old objects key is old-oid + '-' + new-oid

class S3ProbableDeleteRecord {
  std::string record_key;
  struct m0_uint128 old_object_oid;  // oid of old object, if current object is
                                     // replacing old object.
  std::string object_key_in_index;   // current object key, candidate for
                                     // probable delete
  struct m0_uint128 current_object_oid;
  int object_layout_id;                   // current object layout id
  struct m0_uint128 object_list_idx_oid;  // multipart idx in case of multipart
  struct m0_uint128 objects_version_list_idx_oid;
  std::string version_key_in_index;  // Version key for current object
  bool force_delete;  // when force_delete = true, background delete will simply
                      // delete object
  bool is_multipart;
  struct m0_uint128 part_list_idx_oid;  // present in case of multipart

 public:
  S3ProbableDeleteRecord(std::string rec_key, struct m0_uint128 old_oid,
                         std::string obj_key_in_index,
                         struct m0_uint128 new_oid, int layout_id,
                         struct m0_uint128 obj_list_idx_oid,
                         struct m0_uint128 objs_version_list_idx_oid,
                         std::string ver_key_in_index, bool force_del = false,
                         bool is_multipart = false,
                         struct m0_uint128 part_list_oid = {0ULL, 0ULL});
  virtual ~S3ProbableDeleteRecord() {}

  virtual const std::string& get_key() const { return record_key; }

  virtual struct m0_uint128 get_current_object_oid() const {
    return current_object_oid;
  }

  // Override force delete flag
  virtual void set_force_delete(bool flag) { force_delete = flag; }

  virtual std::string to_json();
};

#endif

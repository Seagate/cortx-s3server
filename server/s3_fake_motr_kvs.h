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

#ifndef __S3_SERVER_FAKE_MOTR_KVS__H__
#define __S3_SERVER_FAKE_MOTR_KVS__H__

#include "s3_motr_context.h"

#include <memory>
#include <string>

class S3FakeMotrKvs {
 private:
  S3FakeMotrKvs();

  typedef std::map<std::string, std::string> KeyVal;
  struct Uint128Comp {
    bool operator()(struct m0_uint128 const &a,
                    struct m0_uint128 const &b) const {
      return std::memcmp((void *)&a, (void *)&b, sizeof(a)) < 0;
    }
  };

  std::map<struct m0_uint128, KeyVal, Uint128Comp> in_mem_kv;

 private:
  static std::unique_ptr<S3FakeMotrKvs> inst;

 public:
  virtual ~S3FakeMotrKvs() {}

  int kv_read(struct m0_uint128 const &oid,
              struct s3_motr_kvs_op_context const &kv);

  int kv_next(struct m0_uint128 const &oid,
              struct s3_motr_kvs_op_context const &kv);

  int kv_write(struct m0_uint128 const &oid,
               struct s3_motr_kvs_op_context const &kv);

  int kv_del(struct m0_uint128 const &oid,
             struct s3_motr_kvs_op_context const &kv);

 public:
  static S3FakeMotrKvs *instance() {
    if (!inst) {
      inst = std::unique_ptr<S3FakeMotrKvs>(new S3FakeMotrKvs());
    }
    return inst.get();
  }
};

#endif

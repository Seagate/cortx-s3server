

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

#include "s3_fake_motr_kvs.h"
#include "s3_log.h"

#include <algorithm>

std::unique_ptr<S3FakeMotrKvs> S3FakeMotrKvs::inst;

S3FakeMotrKvs::S3FakeMotrKvs() : in_mem_kv() {}

int S3FakeMotrKvs::kv_read(struct m0_uint128 const &oid,
                           struct s3_motr_kvs_op_context const &kv) {
  s3_log(S3_LOG_DEBUG, "", "Entering with oid %" SCNx64 " : %" SCNx64 "\n",
         oid.u_hi, oid.u_lo);
  if (in_mem_kv.count(oid) == 0) {
    s3_log(S3_LOG_DEBUG, "", "Exiting NOENT\n");
    return -ENOENT;
  }

  KeyVal &obj_kv = in_mem_kv[oid];
  int cnt = kv.values->ov_vec.v_nr;
  for (int i = 0; i < cnt; ++i) {
    std::string search_key((char *)kv.keys->ov_buf[i],
                           kv.keys->ov_vec.v_count[i]);
    if (obj_kv.count(search_key) == 0) {
      kv.rcs[i] = -ENOENT;
      s3_log(S3_LOG_DEBUG, "", "k:>%s v:>ENOENT\n", search_key.c_str());
      continue;
    }

    std::string found_val = obj_kv[search_key];
    kv.rcs[i] = 0;
    kv.values->ov_vec.v_count[i] = found_val.length();
    kv.values->ov_buf[i] = strdup(found_val.c_str());
    s3_log(S3_LOG_DEBUG, "", "k:>%s v:>%s\n", search_key.c_str(),
           found_val.c_str());
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting 0\n");
  return 0;
}

int S3FakeMotrKvs::kv_next(struct m0_uint128 const &oid,
                           struct s3_motr_kvs_op_context const &kv) {
  s3_log(S3_LOG_DEBUG, "", "Entering with oid %" SCNx64 " : %" SCNx64 "\n",
         oid.u_hi, oid.u_lo);
  if (in_mem_kv.count(oid) == 0) {
    s3_log(S3_LOG_DEBUG, "", "Exiting ENOENT\n");
    return -ENOENT;
  }

  KeyVal &obj_kv = in_mem_kv[oid];
  int cnt = kv.values->ov_vec.v_nr;
  auto val_it = std::begin(obj_kv);
  if (kv.keys->ov_vec.v_count[0] > 0) {
    std::string search_key((char *)kv.keys->ov_buf[0],
                           kv.keys->ov_vec.v_count[0]);

    kv.keys->ov_vec.v_count[0] = 0;
    // do not free - done in upper level
    kv.keys->ov_buf[0] = nullptr;

    val_it = obj_kv.find(search_key);
    if (val_it == std::end(obj_kv)) {
      val_it = std::find_if(
          std::begin(obj_kv), std::end(obj_kv),
          [&search_key](const std::pair<std::string, std::string> & itv)
              ->bool { return itv.first.find(search_key) == 0; });
    } else {
      // found full value, should take next
      ++val_it;
    }
    if (val_it == std::end(obj_kv)) {
      s3_log(S3_LOG_DEBUG, "", "Exiting k:>%s ENOENT\n", search_key.c_str());
      return -ENOENT;
    }
    s3_log(S3_LOG_DEBUG, "", "Initial k:>%s found\n", search_key.c_str());
  }

  for (int i = 0; i < cnt; ++i) {
    kv.rcs[i] = -ENOENT;
    if (val_it == std::end(obj_kv)) {
      continue;
    }

    kv.rcs[i] = 0;

    std::string tmp = val_it->first;
    kv.keys->ov_vec.v_count[i] = tmp.length();
    kv.keys->ov_buf[i] = strdup(tmp.c_str());

    tmp = val_it->second;
    kv.values->ov_vec.v_count[i] = tmp.length();
    kv.values->ov_buf[i] = strdup(tmp.c_str());

    s3_log(S3_LOG_DEBUG, "", "Got k:>%s v:>%s\n", (char *)kv.keys->ov_buf[i],
           (char *)kv.values->ov_buf[i]);

    ++val_it;
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting 0\n");
  return 0;
}

int S3FakeMotrKvs::kv_write(struct m0_uint128 const &oid,
                            struct s3_motr_kvs_op_context const &kv) {
  s3_log(S3_LOG_DEBUG, "", "Entering with oid %" SCNx64 " : %" SCNx64 "\n",
         oid.u_hi, oid.u_lo);
  KeyVal &obj_kv = in_mem_kv[oid];

  int cnt = kv.values->ov_vec.v_nr;
  for (int i = 0; i < cnt; ++i) {
    std::string nkey((char *)kv.keys->ov_buf[i], kv.keys->ov_vec.v_count[i]);
    std::string nval((char *)kv.values->ov_buf[i],
                     kv.values->ov_vec.v_count[i]);
    obj_kv[nkey] = nval;
    kv.rcs[i] = 0;

    s3_log(S3_LOG_DEBUG, "", "Add k:>%s -> v:>%s\n", nkey.c_str(),
           nval.c_str());
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting 0\n");
  return 0;
}

int S3FakeMotrKvs::kv_del(struct m0_uint128 const &oid,
                          struct s3_motr_kvs_op_context const &kv) {
  s3_log(S3_LOG_DEBUG, "", "Entering with oid %" SCNx64 " : %" SCNx64 "\n",
         oid.u_hi, oid.u_lo);
  if (in_mem_kv.count(oid) == 0) {
    s3_log(S3_LOG_DEBUG, "", "Exiting NOENT\n");
    return -ENOENT;
  }

  KeyVal &obj_kv = in_mem_kv[oid];

  int cnt = kv.values->ov_vec.v_nr;
  for (int i = 0; i < cnt; ++i) {
    std::string nkey((char *)kv.keys->ov_buf[i], kv.keys->ov_vec.v_count[i]);
    kv.rcs[i] = -ENOENT;
    if (obj_kv.erase(nkey) > 0) {
      kv.rcs[i] = 0;
    }

    s3_log(S3_LOG_DEBUG, "", "Del k:>%s -> %d\n", nkey.c_str(), kv.rcs[i]);
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting 0\n");
  return 0;
}

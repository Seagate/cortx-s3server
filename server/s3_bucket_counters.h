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

#pragma once

#ifndef __S3_SERVER_S3_BUCKET_COUNTERS_H__
#define __S3_SERVER_S3_BUCKET_COUNTERS_H__

#include <string>
#include <functional>
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"
#include "s3_factory.h"

#define OBJECT_COUNT "Object_count"
#define TOTAL_SIZE "Total_size"
#define DEGRADED_COUNT "Degraded_count"

class S3BucketObjectCounter {

 private:
  std::string request_id;
  std::string bucket_name;
  std::string key;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kv_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3RequestObject> req;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  uint64_t saved_object_count;
  uint64_t saved_total_size;
  uint64_t saved_degraded_count;
  uint64_t inc_object_count;
  uint64_t inc_total_size;
  uint64_t inc_degraded_count;
  bool is_cache_created;

 public:
  S3BucketObjectCounter(
      std::shared_ptr<S3RequestObject> request, std::string bkt_name,
      std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<MotrAPI> motr_api = nullptr);
  void save(std::function<void(void)> on_success,
            std::function<void(void)> on_failed);
  void add_inc_object_count(uint64_t obj_count);
  void add_inc_size(uint64_t size);
  void add_inc_degraded_count(uint64_t degraded_count);
  void save_counters_successful();
  void save_metadata_failed();
  void load_successful();
  void load_failed();
  void save_bucket_counters();
  std::string to_json();
  int from_json(std::string content);
};

#endif  //__S3_SERVER_S3_BUCKET_COUNTERS_H__
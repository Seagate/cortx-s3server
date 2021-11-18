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

class S3BucketObjectCounter {

 private:
  std::string request_id;
  std::string bucket_name;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kv_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3RequestObject> req;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

 public:
  S3BucketObjectCounter(
      std::shared_ptr<S3RequestObject> request, std::string bkt_name,
      std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<MotrAPI> motr_api = nullptr);
  void save(std::function<void(void)> on_success,
            std::function<void(void)> on_failed);
  std::string to_json();
  void save_counters_successful();
  void save_metadata_failed();
};

#endif  //__S3_SERVER_S3_BUCKET_COUNTERS_H__
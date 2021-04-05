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

#include "s3_bucket_metadata.h"

class S3BucketMetadataProxy : public S3BucketMetadata {
 public:
  S3BucketMetadataProxy(std::shared_ptr<S3RequestObject> req,
                        const std::string& bucket);
  ~S3BucketMetadataProxy() override;

  // The class doesn't make requests to Motr directly.
  // All four operations below are forwarded to bucket metadata cache.
  void load(std::function<void(void)> on_success,
            std::function<void(void)> on_failed) override;

  void save(std::function<void(void)> on_success,
            std::function<void(void)> on_failed) override;

  void update(std::function<void(void)> on_success,
              std::function<void(void)> on_failed) override;

  void remove(std::function<void(void)> on_success,
              std::function<void(void)> on_failed) override;

  // Don't use explicitly except for UTs
  void initialize();
  void set_state(S3BucketMetadataState state) { this->state = state; }

 private:
  void on_load(S3BucketMetadataState state, const S3BucketMetadata& src);
  void on_save(S3BucketMetadataState state);
  void on_update(S3BucketMetadataState state);
  void on_remove(S3BucketMetadataState state);

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;
};

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

#ifndef __S3_SERVER_S3_BUCKET_REMOTE_ADD_ACTION_H__
#define __S3_SERVER_S3_BUCKET_REMOTE_ADD_ACTION_H__

#include <memory>
#include <string>

#include "s3_bucket_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"

class S3BucketRemoteAddAction : public S3BucketAction {
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3BucketRemoteAddBodyFactory> bucket_remote_add_body_factory;
  std::shared_ptr<S3BucketRemoteAddBody> bucket_remote_add_body;
  std::string req_content;
  std::string alias_name, creds_str;

 public:
  S3BucketRemoteAddAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<S3BucketRemoteAddBodyFactory> bucket_body_factory =
          nullptr);

  void setup_steps();
  void validate_incoming_request();
  void consume_incoming_content();
  void validate_request_body(std::string content);
  void add_remote_bucket_creds_to_bucket_metadata();
  void add_remote_bucket_creds_to_bucket_metadata_failed();
  void fetch_bucket_info_failed();
  void send_response_to_s3_client();

  // To Do: Add unit test cases
};

#endif

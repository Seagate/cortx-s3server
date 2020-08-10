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

#ifndef __S3_SERVER_S3_SERVICE_LIST_RESPONSE_H__
#define __S3_SERVER_S3_SERVICE_LIST_RESPONSE_H__

#include <memory>
#include <string>
#include <vector>

#include "s3_bucket_metadata.h"

class S3ServiceListResponse {
  std::string owner_name;
  std::string owner_id;
  std::vector<std::shared_ptr<S3BucketMetadata>> bucket_list;

  // Generated xml response
  std::string response_xml;

 public:
  S3ServiceListResponse();
  void set_owner_name(std::string name);
  void set_owner_id(std::string id);
  void add_bucket(std::shared_ptr<S3BucketMetadata> bucket);
  std::string& get_xml();
  int get_bucket_count() const { return bucket_list.size(); }
};

#endif

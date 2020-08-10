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

#ifndef __S3_SERVER_S3_DELETE_MULTIPLE_OBJECTS_BODY_H__
#define __S3_SERVER_S3_DELETE_MULTIPLE_OBJECTS_BODY_H__

#include <string>
#include <vector>

class S3DeleteMultipleObjectsBody {
  std::string xml_content;
  bool is_valid;

  // std::vector<std::unique_ptr<DeleteObjectInfo>> object_list;
  // Version id are yet to be implemented. The order should be maintained
  // Split in 2 for faster access.
  std::vector<std::string> object_keys;
  std::vector<std::string> version_ids;
  bool quiet;

  bool parse_and_validate();

 public:
  S3DeleteMultipleObjectsBody();
  void initialize(std::string& xml);

  bool isOK();

  // returns the count of objects in request to be deleted.
  int get_count() {
    return object_keys.size();
    // return object_list.size();
  }

  std::vector<std::string> get_keys(size_t index, size_t count) {
    if (index + count <= object_keys.size()) {
      std::vector<std::string> sub(object_keys.begin() + index,
                                   object_keys.begin() + index + count);
      return sub;
    } else if (index < object_keys.size()) {
      std::vector<std::string> sub(object_keys.begin() + index,
                                   object_keys.end());
      return sub;
    } else {
      std::vector<std::string> sub;
      return sub;
    }
  }

  std::vector<std::string> get_version_ids(size_t index, size_t count) {
    if (index + count <= version_ids.size()) {
      std::vector<std::string> sub(version_ids.begin() + index,
                                   version_ids.begin() + index + count);
      return sub;
    } else if (index < version_ids.size()) {
      std::vector<std::string> sub(version_ids.begin() + index,
                                   object_keys.end());
      return sub;
    } else {
      std::vector<std::string> sub;
      return sub;
    }
  }

  bool is_quiet() { return quiet; }
};

#endif

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

#ifndef __S3_SERVER_S3_MOTR_LAYOUT_H__
#define __S3_SERVER_S3_MOTR_LAYOUT_H__

#include <map>
#include <utility>

class S3MotrLayoutMap {
  // Map<Layout_id, Unit_size>
  std::map<int, size_t> layout_map;

  // map of up_to_object_size _use_ layout_id; sorted by size
  std::map<size_t, int> obj_layout_map;

  // object size beyond which fixed layout_id is used
  size_t obj_size_cap;
  int layout_id_cap;
  int best_layout_id;

  static S3MotrLayoutMap* instance;

  S3MotrLayoutMap();

 public:
  // Loads config file with layout recommendations
  bool load_layout_recommendations(std::string filename);

  // Returns the <layout id and unit size> recommended for give object size.
  int get_layout_for_object_size(size_t);

  int get_unit_size_for_layout(int id) { return layout_map[id]; }

  int get_best_layout_for_object_size();

  static S3MotrLayoutMap* get_instance() {
    if (!instance) {
      instance = new S3MotrLayoutMap();
    }
    return instance;
  }

  static void destroy_instance() {
    if (instance) {
      delete instance;
      instance = NULL;
    }
  }
};

#endif

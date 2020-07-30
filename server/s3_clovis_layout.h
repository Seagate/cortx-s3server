#pragma once

#ifndef __S3_SERVER_S3_CLOVIS_LAYOUT_H__
#define __S3_SERVER_S3_CLOVIS_LAYOUT_H__

#include <map>
#include <utility>

class S3ClovisLayoutMap {
  // Map<Layout_id, Unit_size>
  std::map<int, size_t> layout_map;

  // map of up_to_object_size _use_ layout_id; sorted by size
  std::map<size_t, int> obj_layout_map;

  // object size beyond which fixed layout_id is used
  size_t obj_size_cap;
  int layout_id_cap;
  int best_layout_id;

  static S3ClovisLayoutMap* instance;

  S3ClovisLayoutMap();

 public:
  // Loads config file with layout recommendations
  bool load_layout_recommendations(std::string filename);

  // Returns the <layout id and unit size> recommended for give object size.
  int get_layout_for_object_size(size_t);

  int get_unit_size_for_layout(int id) { return layout_map[id]; }

  int get_best_layout_for_object_size();

  static S3ClovisLayoutMap* get_instance() {
    if (!instance) {
      instance = new S3ClovisLayoutMap();
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

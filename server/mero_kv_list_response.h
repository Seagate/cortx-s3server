#pragma once

#ifndef __S3_SERVER_MERO_KV_LIST_RESPONSE_H__
#define __S3_SERVER_MERO_KV_LIST_RESPONSE_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <map>

class MeroKVListResponse {
  // value can be url or empty string
  std::string encoding_type;

  std::string index_id;
  std::map<std::string, std::string> kv_list;

  // We use unordered for performance as the keys are already
  // in sorted order as stored in clovis-kv (cassandra).
  std::unordered_set<std::string> common_prefixes;

  // Generated xml response.
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  std::string max_keys;
  bool response_is_truncated;
  std::string next_marker_key;

  std::string get_response_format_key_value(const std::string& key_value);

 public:
  MeroKVListResponse(std::string encoding_type = "");

  void set_index_id(std::string name);
  void set_request_prefix(std::string prefix);
  void set_request_delimiter(std::string delimiter);
  void set_request_marker_key(std::string marker);
  void set_max_keys(std::string count);
  void set_response_is_truncated(bool flag);
  void set_next_marker_key(std::string next);

  void add_kv(const std::string& key, const std::string& value);
  void add_common_prefix(std::string);
  unsigned int size();
  unsigned int common_prefixes_size();

  std::string as_json();

  // Google tests.
};

#endif

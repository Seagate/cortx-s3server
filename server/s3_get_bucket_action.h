#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_GET_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_GET_BUCKET_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_object_list_response.h"
#include "s3_clovis_kvs_reader.h"

class S3GetBucketAction : public S3Action {
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::string last_key;  // last key during each iteration
  S3ObjectListResponse object_list;
  size_t return_list_size;

  bool fetch_successful;

  // Helpers
  std::string get_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name();
  }

  // Request Input params
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  size_t max_keys;

public:
  S3GetBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void get_next_objects();
  void get_next_objects_successful();
  void get_next_objects_failed();

  void send_response_to_s3_client();
};

#endif

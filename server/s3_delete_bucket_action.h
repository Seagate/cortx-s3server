#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_DELETE_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_DELETE_BUCKET_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_kvs_reader.h"

class S3DeleteBucketAction : public S3Action {
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

  std::string last_key;  // last key during each iteration

  bool is_bucket_empty;
  bool delete_successful;

  // Helpers
  std::string get_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name();
  }

public:
  S3DeleteBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void fetch_bucket_metadata();
  void fetch_first_object_metadata();
  void fetch_first_object_metadata_successful();
  void fetch_first_object_metadata_failed();

  void delete_bucket();
  void delete_bucket_successful();
  void delete_bucket_failed();

  void send_response_to_s3_client();
};

#endif

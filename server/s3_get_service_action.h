#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_GET_SERVICE_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_GET_SERVICE_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_service_list_response.h"
#include "s3_clovis_kvs_reader.h"

class S3GetServiceAction : public S3Action {
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::string last_key;  // last key during each iteration
  S3ServiceListResponse bucket_list;

  bool fetch_successful;

  // Helpers
  std::string get_account_user_index_name() {
    return "ACCOUNTUSER/" + request->get_account_name() + "/" + request->get_user_name();
  }

public:
  S3GetServiceAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void get_next_buckets();
  void get_next_buckets_successful();
  void get_next_buckets_failed();

  void send_response_to_s3_client();
};

#endif

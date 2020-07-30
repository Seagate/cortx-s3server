#pragma once

#ifndef __MERO_GET_KEY_VALUE_ACTION_H__
#define __MERO_GET_KEY_VALUE_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_factory.h"
#include "mero_action_base.h"

class MeroGetKeyValueAction : public MeroAction {
  m0_uint128 index_id;
  std::shared_ptr<ClovisAPI> mero_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;

  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;

 public:
  MeroGetKeyValueAction(
      std::shared_ptr<MeroRequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_mero_kvs_reader_factory =
          nullptr);

  void setup_steps();
  void fetch_key_value();
  void fetch_key_value_successful();
  void fetch_key_value_failed();
  void send_response_to_s3_client();
};
#endif
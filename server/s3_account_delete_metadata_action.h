#pragma once

#ifndef __S3_ACCOUNT_DELETE_METADATA_ACTION_H__
#define __S3_ACCOUNT_DELETE_METADATA_ACTION_H__

#include <tuple>
#include <vector>

#include "s3_action_base.h"
#include "s3_factory.h"
#include "s3_put_bucket_body.h"

class S3AccountDeleteMetadataAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::string account_id_from_uri;
  std::string bucket_account_id_key_prefix;

  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;

  void validate_request();

  void fetch_first_bucket_metadata();
  void fetch_first_bucket_metadata_successful();
  void fetch_first_bucket_metadata_failed();

 public:
  S3AccountDeleteMetadataAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr,
      std::shared_ptr<S3ClovisKVSReaderFactory> kvs_reader_factory = nullptr);

  void setup_steps();
  void send_response_to_s3_client();

  // Google Tests
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, Constructor);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, ValidateRequestSuceess);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, ValidateRequestFailed);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, FetchFirstBucketMetadata);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest, FetchFirstBucketMetadataExist);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest,
              FetchFirstBucketMetadataNotExist);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest,
              FetchFirstBucketMetadataMissing);
  FRIEND_TEST(S3AccountDeleteMetadataActionTest,
              FetchFirstBucketMetadataFailed);
};

#endif

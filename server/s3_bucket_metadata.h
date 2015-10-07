#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_BUCKET_METADATA_H__
#define __MERO_FE_S3_SERVER_S3_BUCKET_METADATA_H__

#include <string>
#include <map>
#include <memory>
#include <functional>

#include "s3_request_object.h"
#include "s3_bucket_acl.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"

enum class S3BucketMetadataState {
  empty,    // Initial state, no lookup done
  present,  // Metadata exists and was read successfully
  missing,   // Metadata not present in store.
  saved,    // Metadata saved to store.
  failed
};

class S3BucketMetadata {
  // Holds mainly system-defined metadata (creation date etc)
  // Partially supported on need bases, some of these are placeholders
private:
  std::string account_name;
  std::string user_name;

  // The name for a Bucket
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
  std::string bucket_name;

  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;

  S3BucketACL bucket_ACL;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3BucketMetadataState state;

private:
  // AWS recommends that all bucket names comply with DNS naming convention
  // See Bucket naming restrictions in above link.
  void validate_bucket_name();

  // Any other validations we want to do on metadata
  void validate();
public:
  S3BucketMetadata(std::shared_ptr<S3RequestObject> req);

  // Load attributes
  void add_system_attribute(std::string key, std::string val);
  void add_user_defined_attribute(std::string key, std::string val);

  std::string get_account_index_name() {
    return "ACCOUNT/" + account_name;
  }

  std::string get_account_user_index_name() {
    return "ACCOUNTUSER/" + account_name + "/" + user_name;
  }

  void load(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void load_account_bucket();
  void load_account_bucket_successful();
  void load_account_bucket_failed();
  // load_user_bucket is going to be efficient in terms of cassandra
  // as there will be less data in user row
  void load_user_bucket();
  void load_user_bucket_successful();
  void load_user_bucket_failed();

  void save(std::function<void(void)> on_success, std::function<void(void)> on_failed);

  void create_account_bucket_index();
  void create_account_bucket_index_successful();
  void create_account_bucket_index_failed();
  void create_account_user_bucket_index();
  void create_account_user_bucket_index_successful();
  void create_account_user_bucket_index_failed();

  void save_account_bucket();
  void save_account_bucket_successful();
  void save_account_bucket_failed();
  void save_user_bucket();
  void save_user_bucket_successful();
  void save_user_bucket_failed();

  S3BucketMetadataState get_state() {
    return state;
  }

  // Streaming to json
  std::string to_json();

  void from_json(std::string content);
};

#endif

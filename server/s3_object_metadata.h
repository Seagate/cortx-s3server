
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_OBJECT_METADATA_H__
#define __MERO_FE_S3_SERVER_S3_OBJECT_METADATA_H__

#include <map>
#include <string>
#include <memory>
#include <functional>

#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_request_object.h"
#include "s3_object_acl.h"

enum class S3ObjectMetadataState {
  empty,    // Initial state, no lookup done
  present,  // Metadata exists and was read successfully
  absent,   // Metadata not present in store.
  saved,    // Metadata saved to store.
  failed
};

class S3ObjectMetadata {
  // Holds system-defined metadata (creation date etc)
  // Holds user-defined metadata (names must begin with "x-amz-meta-")
  // Partially supported on need bases, some of these are placeholders
private:
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string user_id;
  std::string bucket_name;
  std::string object_name;

  // The name for a key is a sequence of Unicode characters whose UTF-8 encoding is at most 1024 bytes long. http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-keys
  std::string object_key_uri;

  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;

  S3ObjectACL object_ACL;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ObjectMetadataState state;

private:
  // Any validations we want to do on metadata
  void validate();
public:
  S3ObjectMetadata(std::shared_ptr<S3RequestObject> req);

  std::string get_bucket_index_name() {
    return "BUCKET/" + bucket_name;
  }

  void set_content_length(std::string length);
  size_t get_content_length();
  std::string get_content_length_str();

  void set_md5(std::string md5);
  std::string get_md5();

  // Load attributes
  std::string get_system_attribute(std::string key);
  void add_system_attribute(std::string key, std::string val);
  std::string get_user_defined_attribute(std::string key);
  void add_user_defined_attribute(std::string key, std::string val);

  void load(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void load_successful();
  void load_failed();

  void save(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void create_bucket_index();
  void create_bucket_index_successful();
  void create_bucket_index_failed();
  void save_metadata();
  void save_metadata_successful();
  void save_metadata_failed();

  S3ObjectMetadataState get_state() {
    return state;
  }

  std::string to_json();

  void from_json(std::string content);
};

#endif

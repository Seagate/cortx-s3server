/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __S3_SERVER_S3_OBJECT_METADATA_H__
#define __S3_SERVER_S3_OBJECT_METADATA_H__

#include <gtest/gtest_prod.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "s3_bucket_metadata.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_request_object.h"
#include "s3_timer.h"

enum class S3ObjectMetadataState {
  empty,    // Initial state, no lookup done.
  present,  // Metadata exists and was read successfully.
  missing,  // Metadata not present in store.
  saved,    // Metadata saved to store.
  deleted,  // Metadata deleted from store.
  failed,
  failed_to_launch,  // pre launch operation failed.
  invalid
};

// Forward declarations.
class S3ClovisKVSReaderFactory;
class S3ClovisKVSWriterFactory;
class S3BucketMetadataFactory;

class S3ObjectMetadata {
  // Holds system-defined metadata (creation date etc).
  // Holds user-defined metadata (names must begin with "x-amz-meta-").
  // Partially supported on need bases, some of these are placeholders.
 private:
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string user_id;
  std::string bucket_name;
  std::string object_name;

  std::string request_id;

  std::string upload_id;
  // Maximum retry count for collision resolution.
  unsigned short tried_count;
  std::string salt;

  // The name for a key is a sequence of Unicode characters whose UTF-8 encoding
  // is at most 1024 bytes long.
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-keys
  std::string object_key_uri;

  int layout_id;
  int old_layout_id;

  struct m0_uint128 oid;
  struct m0_uint128 old_oid;
  struct m0_uint128 index_oid;
  struct m0_uint128 part_index_oid;

  std::string mero_oid_str;
  std::string mero_old_oid_str;

  std::string mero_part_oid_str;
  std::string encoded_acl;

  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;

  std::map<std::string, std::string> object_tags;

  bool is_multipart;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kv_reader_factory;
  std::shared_ptr<S3ClovisKVSWriterFactory> clovis_kv_writer_factory;
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ObjectMetadataState state;
  S3Timer s3_timer;

  // `true` in case of json parsing failure.
  bool json_parsing_error;

  void initialize(bool is_multipart, std::string uploadid);
  void collision_detected();
  void create_new_oid();

  // Any validations we want to do on metadata.
  void validate();
  std::string index_name;
  void save_object_list_index_oid_successful();
  void save_object_list_index_oid_failed();

 public:
  S3ObjectMetadata(
      std::shared_ptr<S3RequestObject> req, bool ismultipart = false,
      std::string uploadid = "",
      std::shared_ptr<S3ClovisKVSReaderFactory> kv_reader_factory = nullptr,
      std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr);
  S3ObjectMetadata(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 bucket_idx_id,
      bool ismultipart = false, std::string uploadid = "",
      std::shared_ptr<S3ClovisKVSReaderFactory> kv_reader_factory = nullptr,
      std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory = nullptr,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr);

  struct m0_uint128 get_index_oid();
  std::string get_bucket_index_name() { return "BUCKET/" + bucket_name; }

  std::string get_multipart_index_name() {
    return "BUCKET/" + bucket_name + "/" + "Multipart";
  }

  virtual void set_content_length(std::string length);
  virtual size_t get_content_length();
  virtual size_t get_part_one_size();
  virtual std::string get_content_length_str();

  virtual void set_md5(std::string md5);
  virtual void set_part_one_size(const size_t& part_size);
  virtual std::string get_md5();

  virtual void set_oid(struct m0_uint128 id);
  void set_old_oid(struct m0_uint128 id);
  void acl_from_json(std::string acl_json_str);
  void set_part_index_oid(struct m0_uint128 id);
  virtual struct m0_uint128 get_oid() { return oid; }
  virtual int get_layout_id() { return layout_id; }
  void set_layout_id(int id) { layout_id = id; }
  virtual int get_old_layout_id() { return old_layout_id; }
  void set_old_layout_id(int id) { old_layout_id = id; }

  virtual struct m0_uint128 get_old_oid() { return old_oid; }

  struct m0_uint128 get_part_index_oid() {
    return part_index_oid;
  }

  std::string get_owner_name();
  std::string get_owner_id();
  virtual std::string get_object_name();
  virtual std::string get_user_id();
  virtual std::string get_user_name();
  std::string get_creation_date();
  std::string get_last_modified();
  virtual std::string get_last_modified_gmt();
  virtual std::string get_last_modified_iso();
  virtual void reset_date_time_to_current();
  virtual std::string get_storage_class();
  virtual std::string get_upload_id();
  std::string& get_encoded_object_acl();
  std::string get_acl_as_xml();

  // Load attributes.
  std::string get_system_attribute(std::string key);
  void add_system_attribute(std::string key, std::string val);
  std::string get_user_defined_attribute(std::string key);
  virtual void add_user_defined_attribute(std::string key, std::string val);
  virtual std::map<std::string, std::string>& get_user_attributes() {
    return user_defined_attribute;
  }

  virtual void load(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void load_successful();
  void load_failed();

  virtual void save(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void create_bucket_index();
  void create_bucket_index_successful();
  void create_bucket_index_failed();
  void save_metadata();
  void save_metadata_successful();
  void save_metadata_failed();
  virtual void save_metadata(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed);

  virtual void remove(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);
  void remove_successful();
  void remove_failed();

  virtual S3ObjectMetadataState get_state() { return state; }

  // placeholder state, so as to not perform any operation on this.
  void mark_invalid() { state = S3ObjectMetadataState::invalid; }

  std::string to_json();

  // returns 0 on success, -1 on parsing error.
  virtual int from_json(std::string content);
  virtual void setacl(const std::string& input_acl);
  virtual void set_tags(const std::map<std::string, std::string>& tags_as_map);
  virtual const std::map<std::string, std::string>& get_tags();
  virtual void delete_object_tags();
  virtual std::string get_tags_as_xml();
  virtual bool check_object_tags_exists();
  virtual int object_tags_count();

  virtual std::string create_probable_delete_record(int override_layout_id);

  // Virtual Destructor
  virtual ~S3ObjectMetadata(){};

  // Google tests.
  FRIEND_TEST(S3ObjectMetadataTest, ConstructorTest);
  FRIEND_TEST(S3MultipartObjectMetadataTest, ConstructorTest);
  FRIEND_TEST(S3ObjectMetadataTest, GetSet);
  FRIEND_TEST(S3MultipartObjectMetadataTest, GetUserIdUplodIdName);
  FRIEND_TEST(S3ObjectMetadataTest, GetSetOIDsPolicyAndLocation);
  FRIEND_TEST(S3ObjectMetadataTest, SetAcl);
  FRIEND_TEST(S3ObjectMetadataTest, AddSystemAttribute);
  FRIEND_TEST(S3ObjectMetadataTest, AddUserDefinedAttribute);
  FRIEND_TEST(S3ObjectMetadataTest, Load);
  FRIEND_TEST(S3ObjectMetadataTest, LoadSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, LoadSuccessInvalidJson);
  FRIEND_TEST(S3ObjectMetadataTest, LoadSuccessfulInvalidJson);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedJsonParsingFailed);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataMissing);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataFailed);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMeatdataMissingIndexOID);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMeatdataIndexOIDPresent);
  FRIEND_TEST(S3ObjectMetadataTest, CreateBucketIndex);
  FRIEND_TEST(S3MultipartObjectMetadataTest, CreateBucketIndexSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, CreateBucketIndexSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, SaveObjectListIndexSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, SaveObjectListIndexFailed);
  FRIEND_TEST(S3ObjectMetadataTest, SaveObjectListIndexFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest,
              CreateBucketListIndexFailedCollisionHappened);
  FRIEND_TEST(S3ObjectMetadataTest, CreateBucketListIndexFailed);
  FRIEND_TEST(S3ObjectMetadataTest, CreateBucketListIndexFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, CollisionDetected);
  FRIEND_TEST(S3ObjectMetadataTest, CollisionDetectedMaxAttemptExceeded);
  FRIEND_TEST(S3ObjectMetadataTest, CreateNewOid);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataWithoutParam);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataWithParam);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataSuccess);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataFailed);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, Remove);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveFailed);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, ToJson);
  FRIEND_TEST(S3ObjectMetadataTest, FromJson);
  FRIEND_TEST(S3MultipartObjectMetadataTest, FromJson);
  FRIEND_TEST(S3ObjectMetadataTest, GetEncodedBucketAcl);
};

#endif

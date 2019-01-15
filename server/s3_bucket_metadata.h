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

#ifndef __S3_SERVER_S3_BUCKET_METADATA_H__
#define __S3_SERVER_S3_BUCKET_METADATA_H__

#include <gtest/gtest_prod.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "s3_account_user_index_metadata.h"
#include "s3_bucket_acl.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_log.h"
#include "s3_request_object.h"
#include "s3_request_object.h"

enum class S3BucketMetadataState {
  empty,     // Initial state, no lookup done
  present,   // Metadata exists and was read successfully
  missing,   // Metadata not present in store.
  failed,
  failed_to_launch  // pre launch operation failed
};

// Forward declarations
class S3ClovisKVSReaderFactory;
class S3ClovisKVSWriterFactory;
class S3AccountUserIdxMetadataFactory;

class S3BucketMetadata {
  // Holds mainly system-defined metadata (creation date etc)
  // Partially supported on need bases, some of these are placeholders
 private:
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string user_id;
  std::string salted_bucket_list_index_name;
  std::string salted_object_list_index_name;
  std::string salted_multipart_list_index_name;

  enum class S3BucketMetadataCurrentOp {
    none,
    fetching,
    saving,
    deleting
  };

  // Maximum retry count for collision resolution
  unsigned short collision_attempt_count;
  std::string collision_salt;

  // The name for a Bucket
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
  std::string bucket_name;
  std::string bucket_policy;
  std::map<std::string, std::string> bucket_tags;
  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;

  S3BucketACL bucket_ACL;

  struct m0_uint128 bucket_list_index_oid;

  struct m0_uint128 multipart_index_oid;
  struct m0_uint128 object_list_index_oid;
  std::string object_list_index_oid_u_hi_str;
  std::string object_list_index_oid_u_lo_str;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;

  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<S3ClovisKVSWriterFactory> clovis_kvs_writer_factory;
  std::shared_ptr<S3AccountUserIdxMetadataFactory>
      account_user_index_metadata_factory;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3BucketMetadataState state;
  S3BucketMetadataCurrentOp current_op;

  std::shared_ptr<S3AccountUserIdxMetadata> account_user_index_metadata;

  // `true` in case of json parsing failure
  bool json_parsing_error;

  std::string request_id;

 private:
  std::string get_account_index_id() { return "ACCOUNTUSER/" + account_id; }

  void fetch_bucket_list_index_oid();
  void fetch_bucket_list_index_oid_success();
  void fetch_bucket_list_index_oid_failed();

  void load_bucket_info();
  void load_bucket_info_successful();
  void load_bucket_info_failed();

  void create_bucket_list_index();
  void create_bucket_list_index_successful();
  void create_bucket_list_index_failed();

  void create_object_list_index();
  void create_object_list_index_successful();
  void create_object_list_index_failed();

  void create_multipart_list_index();
  void create_multipart_list_index_successful();
  void create_multipart_list_index_failed();

  void save_bucket_list_index_oid();
  void save_bucket_list_index_oid_successful();
  void save_bucket_list_index_oid_failed();

  std::string create_default_acl();

  void save_bucket_info();
  void save_bucket_info_successful();
  void save_bucket_info_failed();

  void remove_bucket_info();
  void remove_bucket_info_successful();
  void remove_bucket_info_failed();

  // AWS recommends that all bucket names comply with DNS naming convention
  // See Bucket naming restrictions in above link.
  void validate_bucket_name();

  // Any other validations we want to do on metadata
  void validate();
  void handle_collision(std::string base_index_name,
                        std::string& salted_index_name,
                        std::function<void(void)> callback);
  void regenerate_new_index_name(std::string base_index_name,
                                 std::string& salted_index_name);

 public:
  S3BucketMetadata(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_s3_kvs_reader_factory =
          nullptr,
      std::shared_ptr<S3ClovisKVSWriterFactory> clovis_s3_kvs_writer_factory =
          nullptr,
      std::shared_ptr<S3AccountUserIdxMetadataFactory>
          s3_account_user_idx_metadata_factory = nullptr);

  std::string get_bucket_name();
  std::string get_creation_time();
  std::string get_location_constraint();
  std::string get_owner_id();
  std::string get_owner_name();

  std::string& get_encoded_bucket_acl();
  virtual std::string get_tags_as_xml();
  virtual bool check_bucket_tags_exists();
  virtual std::string& get_policy_as_json();
  virtual std::string& get_acl_as_xml();

  virtual struct m0_uint128 get_bucket_list_index_oid();
  virtual struct m0_uint128 get_multipart_index_oid();
  virtual struct m0_uint128 get_object_list_index_oid();
  std::string get_object_list_index_oid_u_hi_str();
  std::string get_object_list_index_oid_u_lo_str();
  void set_bucket_list_index_oid(struct m0_uint128 id);
  void set_multipart_index_oid(struct m0_uint128 id);
  void set_object_list_index_oid(struct m0_uint128 id);
  std::string get_object_list_index_name() { return "BUCKET/" + bucket_name; }
  std::string get_multipart_index_name() {
    return "BUCKET/" + bucket_name + "/" + "Multipart";
  }

  virtual void set_location_constraint(std::string location);

  // Load attributes
  void add_system_attribute(std::string key, std::string val);
  void add_user_defined_attribute(std::string key, std::string val);

  virtual void load(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void load_successful();
  void load_failed();

  virtual void save(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void save_successful();
  void save_failed();

  virtual void setpolicy(std::string& policy_str);
  virtual void set_tags(const std::map<std::string, std::string>& tags_as_map);
  virtual void deletepolicy();
  virtual void delete_bucket_tags();

  virtual void setacl(std::string& acl_str);

  virtual void remove(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);
  void remove_successful();
  void remove_failed();

  // void save_idx_metadata();
  // void save_idx_metadata_successful();
  // void save_idx_metadata_failed();

  virtual S3BucketMetadataState get_state() { return state; }

  // Streaming to json
  std::string to_json();

  // returns 0 on success, -1 on parsing error
  virtual int from_json(std::string content);

  // Google tests
  FRIEND_TEST(S3BucketMetadataTest, ConstructorTest);
  FRIEND_TEST(S3BucketMetadataTest, GetSystemAttributesTest);
  FRIEND_TEST(S3BucketMetadataTest, GetSetOIDsPolicyAndLocation);
  FRIEND_TEST(S3BucketMetadataTest, DeletePolicy);
  FRIEND_TEST(S3BucketMetadataTest, GetTagsAsXml);
  FRIEND_TEST(S3BucketMetadataTest, GetSpecialCharTagsAsXml);
  FRIEND_TEST(S3BucketMetadataTest, AddSystemAttribute);
  FRIEND_TEST(S3BucketMetadataTest, AddUserDefinedAttribute);
  FRIEND_TEST(S3BucketMetadataTest, Load);
  FRIEND_TEST(S3BucketMetadataTest, FetchBucketListIndexOid);
  FRIEND_TEST(S3BucketMetadataTest,
              FetchBucketListIndexOIDSuccessStateIsSaving);
  FRIEND_TEST(S3BucketMetadataTest,
              FetchBucketListIndexOIDSuccessStateIsFetching);
  FRIEND_TEST(S3BucketMetadataTest,
              FetchBucketListIndexOIDSuccessStateIsDeleting);
  FRIEND_TEST(S3BucketMetadataTest, FetchBucketListIndexOIDFailedState);
  FRIEND_TEST(S3BucketMetadataTest, FetchBucketListIndexOIDFailedStateIsSaving);
  FRIEND_TEST(S3BucketMetadataTest,
              FetchBucketListIndexOIDFailedStateIsFetching);
  FRIEND_TEST(S3BucketMetadataTest,
              FetchBucketListIndexOIDFailedIndexMissingStateSaved);
  FRIEND_TEST(S3BucketMetadataTest,
              FetchBucketListIndexOIDFailedIndexFailedStateIsFetching);
  FRIEND_TEST(S3BucketMetadataTest, FetchBucketListIndexOIDFailedIndexFailed);
  FRIEND_TEST(S3BucketMetadataTest, LoadBucketInfo);
  FRIEND_TEST(S3BucketMetadataTest, SetBucketPolicy);
  FRIEND_TEST(S3BucketMetadataTest, SetAcl);
  FRIEND_TEST(S3BucketMetadataTest, LoadBucketInfoFailedJsonParsingFailed);
  FRIEND_TEST(S3BucketMetadataTest, LoadBucketInfoFailedMetadataMissing);
  FRIEND_TEST(S3BucketMetadataTest, LoadBucketInfoFailedMetadataFailed);
  FRIEND_TEST(S3BucketMetadataTest, LoadBucketInfoFailedMetadataFailedToLaunch);
  FRIEND_TEST(S3BucketMetadataTest, SaveMeatdataMissingIndexOID);
  FRIEND_TEST(S3BucketMetadataTest, SaveMeatdataIndexOIDPresent);
  FRIEND_TEST(S3BucketMetadataTest, CreateObjectIndexOIDNotPresent);
  FRIEND_TEST(S3BucketMetadataTest, CreateBucketListIndexCollisionCount0);
  FRIEND_TEST(S3BucketMetadataTest, CreateBucketListIndexCollisionCount1);
  FRIEND_TEST(S3BucketMetadataTest, CreateBucketListIndexSuccessful);
  FRIEND_TEST(S3BucketMetadataTest,
              CreateBucketListIndexFailedCollisionHappened);
  FRIEND_TEST(S3BucketMetadataTest, CreateBucketListIndexFailed);
  FRIEND_TEST(S3BucketMetadataTest, CreateBucketListIndexFailedToLaunch);
  FRIEND_TEST(S3BucketMetadataTest, HandleCollision);
  FRIEND_TEST(S3BucketMetadataTest, HandleCollisionMaxAttemptExceeded);
  FRIEND_TEST(S3BucketMetadataTest, RegeneratedNewIndexName);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketListIndexOid);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketListIndexOidSucessfulStateSaving);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketListIndexOidSucessful);
  FRIEND_TEST(S3BucketMetadataTest,
              SaveBucketListIndexOidSucessfulBcktMetaMissing);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketListIndexOIDFailed);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketListIndexOIDFailedToLaunch);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketInfo);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketInfoSuccess);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketInfoFailed);
  FRIEND_TEST(S3BucketMetadataTest, SaveBucketInfoFailedToLaunch);
  FRIEND_TEST(S3BucketMetadataTest, RemovePresentMetadata);
  FRIEND_TEST(S3BucketMetadataTest, RemoveAfterFetchingBucketListIndexOID);
  FRIEND_TEST(S3BucketMetadataTest, RemoveBucketInfo);
  FRIEND_TEST(S3BucketMetadataTest, RemoveBucketInfoSuccessful);
  FRIEND_TEST(S3BucketMetadataTest, RemoveBucketInfoFailed);
  FRIEND_TEST(S3BucketMetadataTest, RemoveBucketInfoFailedToLaunch);
  FRIEND_TEST(S3BucketMetadataTest, CreateDefaultAcl);
  FRIEND_TEST(S3BucketMetadataTest, ToJson);
  FRIEND_TEST(S3BucketMetadataTest, FromJson);
  FRIEND_TEST(S3BucketMetadataTest, GetEncodedBucketAcl);
  FRIEND_TEST(S3BucketMetadataTest, CreateMultipartListIndexCollisionCount0);
  FRIEND_TEST(S3BucketMetadataTest, CreateObjectListIndexCollisionCount0);
  FRIEND_TEST(S3BucketMetadataTest, CreateObjectListIndexSuccessful);
  FRIEND_TEST(S3BucketMetadataTest,
              CreateObjectListIndexFailedCollisionHappened);
  FRIEND_TEST(S3BucketMetadataTest,
              CreateMultipartListIndexFailedCollisionHappened);
  FRIEND_TEST(S3BucketMetadataTest, CreateObjectListIndexFailed);
  FRIEND_TEST(S3BucketMetadataTest, CreateObjectListIndexFailedToLaunch);
  FRIEND_TEST(S3BucketMetadataTest, CreateMultipartListIndexFailed);
  FRIEND_TEST(S3BucketMetadataTest, CreateMultipartListIndexFailedToLaunch);
};

#endif

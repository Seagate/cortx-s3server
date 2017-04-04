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
  fetching,  // Loading in progress
  present,   // Metadata exists and was read successfully
  missing,   // Metadata not present in store.
  saving,    // Save is in progress
  saved,     // Metadata saved to store.
  deleting,
  deleted,  // Metadata deleted from store
  failed
};

class S3BucketMetadata {
  // Holds mainly system-defined metadata (creation date etc)
  // Partially supported on need bases, some of these are placeholders
 private:
  std::string account_name;
  std::string user_name;
  std::string salted_index_name;

  // Maximum retry count for collision resolution
  unsigned short collision_attempt_count;
  std::string collision_salt;

  // The name for a Bucket
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
  std::string bucket_name;
  std::string bucket_policy;
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

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3BucketMetadataState state;

  std::shared_ptr<S3AccountUserIdxMetadata> account_user_index_metadata;

  // `true` in case of json parsing failure
  bool json_parsing_error;

 private:
  std::string get_account_user_index_name() {
    return "ACCOUNTUSER/" + account_name + "/" + user_name;
  }

  void fetch_bucket_list_index_oid();
  void fetch_bucket_list_index_oid_success();
  void fetch_bucket_list_index_oid_failed();

  void load_bucket_info();
  void load_bucket_info_successful();
  void load_bucket_info_failed();

  void create_bucket_list_index();
  void create_bucket_list_index_successful();
  void create_bucket_list_index_failed();

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
  void handle_collision();
  void regenerate_new_oid();

 public:
  S3BucketMetadata(std::shared_ptr<S3RequestObject> req);

  std::string get_bucket_name();
  std::string get_creation_time();
  std::string get_location_constraint();
  std::string get_owner_id();
  std::string get_owner_name();

  std::string& get_encoded_bucket_acl();
  virtual std::string& get_policy_as_json();
  std::string& get_acl_as_xml();

  struct m0_uint128 get_bucket_list_index_oid();
  virtual struct m0_uint128 get_multipart_index_oid();
  virtual struct m0_uint128 get_object_list_index_oid();
  std::string get_object_list_index_oid_u_hi_str();
  std::string get_object_list_index_oid_u_lo_str();
  void set_bucket_list_index_oid(struct m0_uint128 id);
  void set_multipart_index_oid(struct m0_uint128 id);
  void set_object_list_index_oid(struct m0_uint128 id);

  void set_location_constraint(std::string location);

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

  void setpolicy(std::string& policy_str);
  void deletepolicy();
  void setacl(std::string& acl_str);

  void remove(std::function<void(void)> on_success,
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
  int from_json(std::string content);
};

#endif

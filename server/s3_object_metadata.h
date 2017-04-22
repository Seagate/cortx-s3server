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

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "s3_bucket_metadata.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_object_acl.h"
#include "s3_request_object.h"

enum class S3ObjectMetadataState {
  empty,    // Initial state, no lookup done
  present,  // Metadata exists and was read successfully
  missing,  // Metadata not present in store.
  saved,    // Metadata saved to store.
  deleted,  // Metadata deleted from store
  failed,
  invalid
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

  std::string upload_id;
  // Maximum retry count for collision resolution
  unsigned short tried_count;
  std::string salt;

  // The name for a key is a sequence of Unicode characters whose UTF-8 encoding
  // is at most 1024 bytes long.
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-keys
  std::string object_key_uri;

  struct m0_uint128 oid;
  struct m0_uint128 old_oid;
  struct m0_uint128 index_oid;
  struct m0_uint128 part_index_oid;
  std::string mero_oid_u_hi_str;
  std::string mero_oid_u_lo_str;
  std::string mero_old_oid_u_hi_str;
  std::string mero_old_oid_u_lo_str;

  std::string mero_part_oid_u_hi_str;
  std::string mero_part_oid_u_lo_str;

  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;

  S3ObjectACL object_ACL;
  bool is_multipart;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ObjectMetadataState state;

  // `true` in case of json parsing failure
  bool json_parsing_error;

  void initialize(bool is_multipart, std::string uploadid);
  void collision_detected();
  void create_new_oid();

  // Any validations we want to do on metadata
  void validate();
  std::string index_name;
  void save_object_list_index_oid_successful();
  void save_object_list_index_oid_failed();

 public:
  S3ObjectMetadata(std::shared_ptr<S3RequestObject> req,
                   bool ismultipart = false, std::string uploadid = "");
  S3ObjectMetadata(std::shared_ptr<S3RequestObject> req,
                   struct m0_uint128 bucket_idx_id, bool ismultipart = false,
                   std::string uploadid = "");

  std::string create_default_acl();
  struct m0_uint128 get_index_oid();
  std::string get_bucket_index_name() { return "BUCKET/" + bucket_name; }

  std::string get_multipart_index_name() {
    return "BUCKET/" + bucket_name + "/" + "Multipart";
  }

  virtual void set_content_length(std::string length);
  virtual size_t get_content_length();
  virtual std::string get_content_length_str();

  virtual void set_md5(std::string md5);
  virtual std::string get_md5();

  virtual void set_oid(struct m0_uint128 id);
  void set_old_oid(struct m0_uint128 id);
  void set_part_index_oid(struct m0_uint128 id);
  virtual struct m0_uint128 get_oid() { return oid; }
  struct m0_uint128 get_old_oid() {
    return old_oid;
  }

  struct m0_uint128 get_part_index_oid() {
    return part_index_oid;
  }

  // returns base64 encoded strings
  std::string get_oid_u_hi_str() { return mero_oid_u_hi_str; }
  std::string get_oid_u_lo_str() { return mero_oid_u_lo_str; }

  std::string get_owner_name();
  std::string get_owner_id();
  virtual std::string get_object_name();
  std::string get_user_id();
  std::string get_user_name();
  std::string get_creation_date();
  std::string get_last_modified();
  virtual std::string get_last_modified_gmt();
  std::string get_last_modified_iso();
  std::string get_storage_class();
  virtual std::string get_upload_id();
  std::string& get_encoded_object_acl();
  std::string& get_acl_as_xml();

  // Load attributes
  std::string get_system_attribute(std::string key);
  void add_system_attribute(std::string key, std::string val);
  std::string get_user_defined_attribute(std::string key);
  virtual void add_user_defined_attribute(std::string key, std::string val);
  std::map<std::string, std::string>& get_user_attributes() {
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

  // returns 0 on success, -1 on parsing error
  virtual int from_json(std::string content);
  virtual void setacl(std::string& input_acl_str);
};

#endif

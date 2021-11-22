/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#pragma once

#ifndef __S3_SERVER_S3_BUCKET_METADATA_H__
#define __S3_SERVER_S3_BUCKET_METADATA_H__

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "s3_motr_context.h"

enum class S3BucketMetadataState {
  empty,    // Initial state, no lookup done
  present,  // Metadata exists and was read successfully
  missing,  // Metadata not present in store.
  failed,
  failed_to_launch  // pre launch operation failed
};

// Forward declarations
class S3RequestObject;

class S3BucketMetadata {
  // Holds mainly system-defined metadata (creation date etc)
  // Partially supported on need bases, some of these are placeholders
 protected:
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string owner_canonical_id;
  std::string user_id;
  std::string encoded_acl;

  // The name for a Bucket
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
  std::string bucket_name;
  std::string bucket_policy;
  std::string bucket_versioning_status{"Unversioned"};
  std::map<std::string, std::string> bucket_tags;
  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;

  struct s3_motr_idx_layout multipart_index_layout = {};
  struct s3_motr_idx_layout object_list_index_layout = {};
  struct s3_motr_idx_layout objects_version_list_index_layout = {};
  struct s3_motr_idx_layout extended_metadata_index_layout = {};

  std::shared_ptr<S3RequestObject> request;

  S3BucketMetadataState state = S3BucketMetadataState::empty;

  std::string request_id;
  std::string stripped_request_id;

  const std::string& get_owner_name();

  // Streaming to json
  virtual std::string to_json();

 protected:
  S3BucketMetadata(std::shared_ptr<S3RequestObject> req,
                   const std::string& bucket);

 public:
  virtual ~S3BucketMetadata();

  const std::string& get_request_id() const { return request_id; }
  const std::string& get_bucket_versioning() const {
    return bucket_versioning_status;
  }

  const std::string& get_stripped_request_id() const {
    return stripped_request_id;
  }
  const std::string& get_bucket_name() const;
  const std::string& get_creation_time();
  const std::string& get_location_constraint();
  const std::string& get_owner_id();
  const std::string& get_bucket_owner_account_id();
  const std::string& get_encoded_bucket_acl() const;

  virtual bool check_bucket_tags_exists() const;

  virtual const std::string& get_owner_canonical_id();
  virtual std::string& get_policy_as_json();

  virtual std::string get_tags_as_xml();
  virtual std::string get_acl_as_xml();

  void acl_from_json(std::string acl_json_str);

  virtual const struct s3_motr_idx_layout& get_object_list_index_layout() const;
  virtual const struct s3_motr_idx_layout&
      get_objects_version_list_index_layout() const;
  const struct s3_motr_idx_layout& get_multipart_index_layout() const;
  virtual const struct s3_motr_idx_layout& get_extended_metadata_index_layout()
      const;

  void set_multipart_index_layout(const struct s3_motr_idx_layout& idx_lo);
  void set_object_list_index_layout(const struct s3_motr_idx_layout& idx_lo);
  void set_objects_version_list_index_layout(
      const struct s3_motr_idx_layout& idx_lo);

  virtual void set_location_constraint(std::string location);

  // Load attributes
  void add_system_attribute(std::string key, std::string val);
  void add_user_defined_attribute(std::string key, std::string val);

  virtual void load(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);

  // PUT KV in both global bucket list index and bucket metadata list index.
  virtual void save(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);

  // Update bucket metadata in bucket metadata list index.
  virtual void update(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);

  virtual void setpolicy(std::string policy_str);
  virtual void set_tags(const std::map<std::string, std::string>& tags_as_map);
  virtual void deletepolicy();
  virtual void delete_bucket_tags();
  virtual void set_bucket_versioning(const std::string& bucket_version_status);
  virtual void setacl(const std::string& acl_str);

  virtual void remove(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);

  // returns 0 on success, -1 on parsing error
  virtual int from_json(std::string content);

  virtual S3BucketMetadataState get_state() const { return state; }
};

#endif  // __S3_SERVER_S3_BUCKET_METADATA_H__

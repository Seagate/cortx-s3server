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

#ifndef __S3_SERVER_S3_OBJECT_METADATA_H__
#define __S3_SERVER_S3_OBJECT_METADATA_H__

#include <gtest/gtest_prod.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "s3_bucket_metadata.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"
#include "s3_request_object.h"
#include "s3_timer.h"

#define EXTENDED_METADATA_OBJECT_SEP "|"

enum class S3ObjectMetadataState {
  empty,    // Initial state, no lookup done.
  present,  // Metadata exists and was read successfully.
  missing,  // Metadata not present in store.
  saved,    // Metadata saved to store.
  deleted,  // Metadata deleted from store.
  failed,
  failed_to_launch,  // pre launch operation failed.
  invalid   // Metadata invalid or corrupted
};

struct S3ExtendedObjectInfo {
 public:
  S3ExtendedObjectInfo() {
    start_offset_in_object = 0;
    object_OID = {0, 0};
    object_layout = -1;
    object_pvid = {0, 0};
    total_blocks_in_object = 0;
    object_size = 0;
    requested_object_size = 0;
  }
  size_t start_offset_in_object;
  size_t total_blocks_in_object;
  size_t object_size;
  // Actual length to be returned to client
  size_t requested_object_size;
  struct m0_uint128 object_OID;
  int object_layout;
  struct m0_fid object_pvid;
};

enum class S3ObjectMetadataType {
  simple,           // Object with no parts and/or fragments.
  only_parts,       // Object with only parts.
  only_frgments,    // Object with only fragments.
  parts_fragments,  // Object with parts and fragments.
};

// Forward declarations.
class S3MotrKVSReaderFactory;
class S3MotrKVSWriterFactory;
class S3BucketMetadataFactory;

class S3ObjectMetadataCopyable {

 protected:
  int layout_id = 0;

  std::string request_id;
  std::string stripped_request_id;
  std::string encoded_acl;

  std::map<std::string, std::string> system_defined_attribute;
  std::map<std::string, std::string> user_defined_attribute;
  std::map<std::string, std::string> object_tags;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kv_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
};

// Forward declaration
class S3ObjectExtendedMetadata;

class S3ObjectMetadata : private S3ObjectMetadataCopyable {
  // Holds system-defined metadata (creation date etc).
  // Holds user-defined metadata (names must begin with "x-amz-meta-").
  // Partially supported on need bases, some of these are placeholders.
 private:
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string canonical_id;
  std::string user_id;
  std::string bucket_name;
  std::string object_name;

  // Used in validation
  std::string requested_bucket_name;
  std::string requested_object_name;

  // Reverse epoch time used as version id key in verion index
  std::string rev_epoch_version_id_key;
  // holds base64 encoding value of rev_epoch_version_id_key, this is used
  // in S3 REST APIs as http header x-amz-version-id or query param "VersionId"
  std::string object_version_id;

  std::string upload_id;
  // Maximum retry count for collision resolution.
  unsigned short tried_count = 0;
  std::string salt;

  // The name for a key is a sequence of Unicode characters whose UTF-8 encoding
  // is at most 1024 bytes long.
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-keys
  std::string object_key_uri;

  int old_layout_id = 0;

  struct m0_uint128 oid = M0_ID_APP;
  struct m0_uint128 old_oid = {};
  // Will be object list index oid when simple object upload.
  // Will be multipart object list index oid when multipart object upload.
  struct s3_motr_idx_layout object_list_index_layout = {};
  struct s3_motr_idx_layout objects_version_list_index_layout = {};
  struct s3_motr_idx_layout part_index_layout = {};

  std::string motr_oid_str;
  std::string motr_old_oid_str;
  std::string motr_old_object_version_id;

  std::string motr_part_layout_str;

  bool is_multipart = false;

  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ObjectMetadataState state;
  S3Timer s3_timer;

  void initialize(bool is_multipart, const std::string& uploadid);

  S3ObjectMetadataType obj_type;
  // If multipart, PRTS is non-zero
  unsigned int obj_parts;
  unsigned int obj_fragments;
  std::string pvid_str;
  // Size of primary object, when object is fragmented
  size_t primary_obj_size;
  std::shared_ptr<S3ObjectExtendedMetadata> extended_object_metadata = nullptr;

  // Any validations we want to do on metadata.
  void validate();
  std::string index_name;

  // TODO Eventually move these to s3_common as duplicated in s3_bucket_metadata
  // This index has keys as "object_name"
  std::string get_object_list_index_name() const {
    return "BUCKET/" + bucket_name;
  }
  // This index has keys as "object_name"
  std::string get_multipart_index_name() const {
    return "BUCKET/" + bucket_name + "/" + "Multipart";
  }
  // This index has keys as "object_name/version_id"
  std::string get_version_list_index_name() const {
    return "BUCKET/" + bucket_name + "/objects/versions";
  }

 public:
  S3ObjectMetadata(std::shared_ptr<S3RequestObject> req,
                   bool ismultipart = false, std::string uploadid = "",
                   std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory =
                       nullptr,
                   std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory =
                       nullptr,
                   std::shared_ptr<MotrAPI> motr_api = nullptr);

  S3ObjectMetadata(std::shared_ptr<S3RequestObject> req,
                   std::string bucket_name, std::string object_name,
                   bool ismultipart = false, std::string uploadid = "",
                   std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory =
                       nullptr,
                   std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory =
                       nullptr,
                   std::shared_ptr<MotrAPI> motr_api = nullptr);

  S3ObjectMetadata(const S3ObjectMetadata&);
  S3ObjectMetadataType get_object_type() const { return this->obj_type; }
  // Call these when Object metadata save/remove needs to be called.
  // id can be object list index OID or
  // id can be multipart upload list index OID
  void set_object_list_index_layout(const struct s3_motr_idx_layout& lo);
  void set_objects_version_list_index_layout(
      const struct s3_motr_idx_layout& lo);

  inline const std::shared_ptr<S3ObjectExtendedMetadata>&
  get_extended_object_metadata() {
    return extended_object_metadata;
  }

  inline void set_extended_object_metadata(
      std::shared_ptr<S3ObjectExtendedMetadata> ext_object_metadata) {
    extended_object_metadata = std::move(ext_object_metadata);
  }

  inline size_t get_primary_obj_size() { return primary_obj_size; }

  inline void set_primary_obj_size(size_t obj_size) {
    primary_obj_size = obj_size;
  }

  virtual std::string get_version_key_in_index();
  virtual const struct s3_motr_idx_layout& get_object_list_index_layout() const;
  virtual const struct s3_motr_idx_layout&
      get_objects_version_list_index_layout() const;

  virtual void set_content_length(std::string length);
  virtual size_t get_content_length();
  virtual void rename_object_name(std::string new_object_name);
  virtual std::string get_content_length_str();
  virtual void set_content_type(std::string content_type);
  virtual std::string get_content_type();

  virtual void set_md5(std::string md5);
  virtual std::string get_md5();

  virtual void set_oid(struct m0_uint128 id);
  void set_old_oid(struct m0_uint128 id);
  void acl_from_json(std::string acl_json_str);
  void set_part_index_layout(const struct s3_motr_idx_layout&);
  virtual struct m0_uint128 get_oid() { return oid; }
  virtual int get_layout_id() { return layout_id; }
  void set_layout_id(int id) { layout_id = id; }
  virtual int get_old_layout_id() { return old_layout_id; }
  void set_old_layout_id(int id) { old_layout_id = id; }

  virtual struct m0_uint128 get_old_oid() { return old_oid; }

  const struct s3_motr_idx_layout& get_part_index_layout() const {
    return part_index_layout;
  }

  void regenerate_version_id();

  std::string const get_obj_version_id() { return object_version_id; }
  std::string const get_obj_version_key() { return rev_epoch_version_id_key; }

  void set_version_id(std::string ver_id);

  std::string get_old_obj_version_id() { return motr_old_object_version_id; }
  void set_old_version_id(std::string old_obj_ver_id);

  std::string get_owner_name();
  std::string get_owner_id();
  virtual std::string get_object_name();
  virtual std::string get_bucket_name();
  virtual std::string get_user_id();
  virtual std::string get_user_name();
  virtual std::string get_canonical_id();
  virtual std::string get_account_name();
  std::string get_creation_date();
  std::string get_last_modified();
  virtual std::string get_last_modified_gmt();
  virtual std::string get_last_modified_iso();
  virtual void reset_date_time_to_current();
  virtual std::string get_storage_class();
  virtual std::string get_upload_id();
  std::string& get_encoded_object_acl();
  std::string get_acl_as_xml();

  virtual struct m0_fid get_pvid() const;
  void set_pvid(const struct m0_fid* p_pvid);

  const std::string& get_pvid_str() const { return pvid_str; }
  void set_pvid_str(const std::string& val) { pvid_str = val; }

  inline unsigned int get_number_of_parts() const { return this->obj_parts; }
  inline unsigned int get_number_of_fragments() const {
    return this->obj_fragments;
  }
  inline void set_number_of_parts(unsigned int parts) {
    this->obj_parts = parts;
  }
  inline void set_number_of_fragments(unsigned int fragments) {
    this->obj_fragments = fragments;
  }
  inline bool is_object_extended() {
    return obj_type != S3ObjectMetadataType::simple;
  }
  inline void set_object_type(S3ObjectMetadataType obj_type) {
    this->obj_type = obj_type;
  }
  // Load attributes.
  std::string get_system_attribute(std::string key);
  void add_system_attribute(std::string key, std::string val);
  std::string get_user_defined_attribute(std::string key);
  virtual void add_user_defined_attribute(std::string key, std::string val);
  virtual std::map<std::string, std::string>& get_user_attributes() {
    return user_defined_attribute;
  }

  // Load object metadata from object list index
  virtual void load(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);

  // Save object metadata to versions list index & object list index
  virtual void save(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);

  // Save object metadata ONLY object list index
  virtual void save_metadata(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed);

  // Remove object metadata from object list index & versions list index
  virtual void remove(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);
  virtual void remove_version_metadata(std::function<void(void)> on_success,
                                       std::function<void(void)> on_failed);

  virtual S3ObjectMetadataState get_state() { return state; }

  // placeholder state, so as to not perform any operation on this.
  void mark_invalid() { state = S3ObjectMetadataState::invalid; }

  // For object metadata in object listing
  std::string to_json();
  // For storing minimal version entry in version listing
  std::string version_entry_to_json();

  // returns 0 on success, -1 on parsing error.
  virtual int from_json(std::string content);
  virtual void setacl(const std::string& input_acl);
  virtual void set_tags(const std::map<std::string, std::string>& tags_as_map);
  virtual const std::map<std::string, std::string>& get_tags();
  virtual void delete_object_tags();
  virtual std::string get_tags_as_xml();
  virtual bool check_object_tags_exists();
  virtual int object_tags_count();

  // Virtual Destructor
  virtual ~S3ObjectMetadata() {};

 private:
  // Methods used internally within

  // Load object metadata from object list index
  void load_successful();
  void load_failed();

  // save_metadata saves to object list index
  void save_metadata();
  void save_metadata_successful();
  void save_metadata_failed();

  // save_version_metadata saves to objects "version" list index
  void save_version_metadata();
  void save_version_metadata_successful();
  void save_version_metadata_failed();

  void save_extended_metadata_successful();
  void save_extended_metadata_failed();

  // Remove entry from object list index
  void remove_object_metadata();
  void remove_object_metadata_successful();
  void remove_object_metadata_failed();

  // Remove entry from objects version list index
  void remove_version_metadata();
  void remove_version_metadata_successful();
  void remove_version_metadata_failed();

  // Validate just read metadata
  bool validate_attrs();

 public:
  // Google tests.
  FRIEND_TEST(S3ObjectMetadataTest, ConstructorTest);
  FRIEND_TEST(S3MultipartObjectMetadataTest, ConstructorTest);
  FRIEND_TEST(S3ObjectMetadataTest, GetSet);
  FRIEND_TEST(S3ObjectMetadataTest, RenameObjectName);
  FRIEND_TEST(S3MultipartObjectMetadataTest, GetUserIdUplodIdName);
  FRIEND_TEST(S3ObjectMetadataTest, GetSetOIDsPolicyAndLocation);
  FRIEND_TEST(S3ObjectMetadataTest, SetAcl);
  FRIEND_TEST(S3ObjectMetadataTest, AddSystemAttribute);
  FRIEND_TEST(S3ObjectMetadataTest, AddUserDefinedAttribute);
  FRIEND_TEST(S3ObjectMetadataTest, Load);
  FRIEND_TEST(S3ObjectMetadataTest, LoadSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, LoadMetadataFail);
  FRIEND_TEST(S3ObjectMetadataTest, LoadSuccessInvalidJson);
  FRIEND_TEST(S3ObjectMetadataTest, LoadSuccessfulInvalidJson);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedJsonParsingFailed);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataMissing);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataFailed);
  FRIEND_TEST(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMeatdataMissingIndexOID);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMeatdataIndexOIDPresent);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataWithoutParam);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataWithParam);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataSuccess);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataFailed);
  FRIEND_TEST(S3ObjectMetadataTest, SaveMetadataFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, Remove);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveObjectMetadataSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveVersionMetadataSuccessful);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveObjectMetadataFailed);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveObjectMetadataFailedToLaunch);
  FRIEND_TEST(S3ObjectMetadataTest, RemoveVersionMetadataFailed);
  FRIEND_TEST(S3ObjectMetadataTest, ToJson);
  FRIEND_TEST(S3ObjectMetadataTest, FromJson);
  FRIEND_TEST(S3MultipartObjectMetadataTest, FromJson);
  FRIEND_TEST(S3ObjectMetadataTest, GetEncodedBucketAcl);
  FRIEND_TEST(S3DeleteObjectActionTest, CleanupOnMetadataDeletion);
};

// Fragment or the part detail structure
struct s3_part_frag_context {
  struct m0_uint128 motr_OID;
  struct m0_fid PVID;
  std::string versionID;
  size_t item_size;
  int layout_id;
  bool is_multipart;
};

// TBD
// Class to read/write S3 object parts and fragments from object list index
// e.g., see object list table entry below:
// ObjectOne                  <Basic object metadata> + versionID, FNo, PRTS
// ObjectTwo                  <Basic object metadata> + versionID, FNo, PRTS
// ~ObjectOne|versionID|F1    OID1, PVID, size, layout-id1
// ~ObjectTwo|versionID|P1    OID2, PVID, size, layout-id2
// ~ObjectTwo|versionID|P2|F1 OID3, PVID, size, layout-id3
// ~ObjectTwo|versionID|P2|F2 OID4, PVID, size, layout-id4
class S3ObjectExtendedMetadata : private S3ObjectMetadataCopyable {
 private:
  std::string bucket_name;
  std::string object_name;
  std::string last_object;
  std::string version_id;
  struct s3_motr_idx_layout extended_list_index_layout = {};
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  unsigned int fragments;
  unsigned int parts;
  std::vector<std::string> extended_keys;
  S3ObjectMetadataState state;
  // Total size of all fragments/parts
  size_t total_size;
  // Key is: Multipart number (in case of fragments of multipart, e.g, 1, 2,
  // etc), OR,
  // 0 if fragments of simple object.
  std::map<int, std::vector<struct s3_part_frag_context>> ext_objects;

  virtual void get_obj_ext_entries(std::string last_object);
  virtual int from_json(std::string key, std::string content);
  virtual std::string get_json_str(struct s3_part_frag_context& frag_entry);

 protected:
  void save_extended_metadata();
  void save_extended_metadata_successful();
  void save_extended_metadata_failed();

 public:
  // 'no_of_parts' and 'no_of_fragments' set to default 0, can be used while
  // creating a fresh extended object.
  S3ObjectExtendedMetadata(
      std::shared_ptr<S3RequestObject> req, const std::string& bucketname,
      const std::string& objectname, const std::string& versionid,
      unsigned int no_of_parts = 0, unsigned int no_of_fragments = 0,
      std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory = nullptr,
      std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory = nullptr,
      std::shared_ptr<MotrAPI> motr_api = nullptr);

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  // Virtual Destructor
  virtual ~S3ObjectExtendedMetadata() {};
  virtual bool has_entries() { return (ext_objects.size() > 0) ? true : false; }
  virtual inline const std::map<int, std::vector<struct s3_part_frag_context>>&
  get_raw_extended_entries() {
    return ext_objects;
  }

  virtual unsigned int get_fragment_count() { return fragments; }
  virtual unsigned int get_part_count() { return parts; }
  virtual void load(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  virtual void save(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  virtual void set_extended_list_index_layout(
      const struct s3_motr_idx_layout& lo);
  virtual const struct s3_motr_idx_layout& get_extended_list_index_layout()
      const;
  void set_version_id(const std::string& versionid) { version_id = versionid; }
  void get_obj_ext_entries_failed();
  void get_obj_ext_entries_successful();
  inline size_t get_size() { return total_size; }
  // Returns extended key/value pair of entries of object with fragments/parts
  virtual std::map<std::string, std::string> get_kv_list_of_extended_entries();
  // Adds an extended entry, when object write fails due to degradation
  // For first part, part_no=1 and for first fragment on part, fragment_no=1
  virtual void add_extended_entry(struct s3_part_frag_context& part_frag_ctx,
                                  unsigned int fragment_no,
                                  unsigned int part_no);
  void set_size_of_extended_entry(size_t fragment_size,
                                  unsigned int fragment_no,
                                  unsigned int part_no);

  // Delete extended metadata entries
  virtual void remove(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);

  void remove_ext_object_metadata();
  void remove_ext_object_metadata_successful();
  void remove_ext_object_metadata_failed();
  virtual std::vector<std::string> get_extended_entries_key_list();

 public:
  // Google tests.
  FRIEND_TEST(S3ObjectExtendedMetadataTest, ConstructorTest);
  FRIEND_TEST(S3ObjectExtendedMetadataTest,
              AddExtendedEntrySingleFragmentWithoutPartTest);
  FRIEND_TEST(S3ObjectExtendedMetadataTest,
              AddExtendedEntryMultiFragmentWithoutPartTest);
  FRIEND_TEST(S3ObjectExtendedMetadataTest,
              AddExtendedEntrySingleFragmentSinglePartTest);
  FRIEND_TEST(S3ObjectExtendedMetadataTest,
              AddExtendedEntryMultiFragmentSinglePartTest);
  FRIEND_TEST(S3ObjectExtendedMetadataTest,
              AddExtendedEntrySingleFragmentMultiPartTest);
  FRIEND_TEST(S3ObjectExtendedMetadataTest,
              AddExtendedEntryMultiFragmentMultiPartTest);
};

#endif

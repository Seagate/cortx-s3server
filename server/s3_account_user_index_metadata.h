/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 20-July-2016
 */

#pragma once

#ifndef __S3_SERVER_S3_ACCOUNT_USER_METADATA_H__
#define __S3_SERVER_S3_ACCOUNT_USER_METADATA_H__

#include <string>

#include "gtest/gtest_prod.h"
#include "s3_bucket_acl.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_log.h"
#include "s3_request_object.h"

// Forward declarations
class S3ClovisKVSReaderFactory;
class S3ClovisKVSWriterFactory;

enum class S3AccountUserIdxMetadataState {
  empty,  // Initial state, no lookup done

  // Ops on root index
  created,  // create root success
  exists,   // create root exists

  // Following are ops on key-val
  present,  // Metadata exists and was read successfully
  missing,  // Metadata not present in store.
  saved,    // Metadata saved to store.
  deleted,  // Metadata deleted from store
  failed
};

class S3AccountUserIdxMetadata {
  // Holds mainly system-defined metadata
  // Partially supported on need basis, some of these are placeholders
  // ACCOUNTUSERINDEX  (Account User Index)
  //    Key = "ACCOUNTUSER/<Account id>" | Value = "Account user Index
  //    OID", account and user info
  // "Account user Index OID" = OID of index containing bucket list
 private:
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string user_id;

  // The name for a AccountUserIndex
  std::string account_user_index_name;
  std::string request_id;
  struct m0_uint128 bucket_list_index_oid;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;
  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<S3ClovisKVSWriterFactory> clovis_kvs_writer_factory;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3AccountUserIdxMetadataState state;

  // `true` in case of json parsing failure
  bool json_parsing_error;

 private:
  std::string get_account_index_id() { return "ACCOUNTUSER/" + account_id; }

 public:
  S3AccountUserIdxMetadata(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> s3_clovis_apii = nullptr,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_s3_kvs_reader_factory =
          nullptr,
      std::shared_ptr<S3ClovisKVSWriterFactory> clovis_s3_kvs_writer_factory =
          nullptr);

  std::string get_account_name();
  std::string get_account_id();

  std::string get_user_name();
  std::string get_user_id();

  struct m0_uint128 get_bucket_list_index_oid();
  void set_bucket_list_index_oid(struct m0_uint128 id);

  // Load attributes
  void add_system_attribute(std::string key, std::string val);
  void add_user_defined_attribute(std::string key, std::string val);

  // Blocking operation.
  // Will only create index once with a reserved OID
  void create_root_account_user_index();

  // Load Account user info(bucket list oid)
  virtual void load(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  virtual void load_successful();
  virtual void load_failed();
  virtual ~S3AccountUserIdxMetadata() {}

  // Save Account user info(bucket list oid)
  virtual void save(std::function<void(void)> on_success,
                    std::function<void(void)> on_failed);
  void save_successful();
  void save_failed();

  // Remove Account user info(bucket list oid)
  virtual void remove(std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);
  void remove_successful();
  void remove_failed();

  virtual S3AccountUserIdxMetadataState get_state() { return state; }

  // Streaming to/from json
  std::string to_json();
  // returns 0 on success, -1 on parsing error
  int from_json(std::string content);

  // For Google mocks
  FRIEND_TEST(S3AccountUserIdxMetadataTest, Constructor);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, Load);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, LoadSuccessful);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, LoadSuccessfulJsonError);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, LoadFailed);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, LoadFailedMissing);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, Save);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, SaveSuccessful);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, SaveFailed);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, Remove);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, RemoveSuccessful);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, RemoveFailed);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, ToJson);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, FromJson);
  FRIEND_TEST(S3AccountUserIdxMetadataTest, FromJsonError);
};

#endif

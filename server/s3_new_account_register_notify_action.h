/*
 * COPYRIGHT 2018 SEAGATE LLC
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
 * Original author:  Prashanth vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 16-June-2018
 */

#pragma once

#ifndef __S3_NEW_ACCOUNT_REGISTER_NOTIFY_ACTION_H__
#define __S3_NEW_ACCOUNT_REGISTER_NOTIFY_ACTION_H__
#include <gtest/gtest_prod.h>
#include <tuple>
#include <vector>

#include "s3_action_base.h"
#include "s3_account_user_index_metadata.h"
#include "s3_factory.h"
#include "s3_put_bucket_body.h"

class S3NewAccountRegisterNotifyAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3AccountUserIdxMetadataFactory>
      account_user_index_metadata_factory;
  std::shared_ptr<S3AccountUserIdxMetadata> account_user_index_metadata;

  std::string account_id_from_uri;
  struct m0_uint128 bucket_list_index_oid;

  std::string salted_bucket_list_index_name;

  // Maximum retry count for collision resolution
  unsigned short collision_attempt_count;
  std::string collision_salt;

  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;

  std::shared_ptr<S3ClovisKVSWriterFactory> clovis_kvs_writer_factory;

  std::string get_account_index_id() {
    return "ACCOUNTUSER/" + account_id_from_uri;
  }

  void validate_request();
  void validate_fetched_bucket_list_index_oid();
  void fetch_bucket_list_index_oid();

  void create_bucket_list_index();
  void create_bucket_list_index_successful();
  void create_bucket_list_index_failed();

  void save_bucket_list_index_oid();
  void save_bucket_list_index_oid_successful();
  void save_bucket_list_index_oid_failed();

  void handle_collision(std::string base_index_name,
                        std::string& salted_index_name,
                        std::function<void(void)> callback);
  void regenerate_new_index_name(std::string base_index_name,
                                 std::string& salted_index_name);

 public:
  S3NewAccountRegisterNotifyAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr,
      std::shared_ptr<S3ClovisKVSWriterFactory> kvs_writer_factory = nullptr,
      std::shared_ptr<S3AccountUserIdxMetadataFactory>
          s3_account_user_idx_metadata_factory = nullptr);

  void setup_steps();
  void send_response_to_s3_client();

  // Google Tests
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, Constructor);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, FetchBucketListIndexOid);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              ValidateFetchedBucketListIndexOidAlreadyPresent);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              ValidateFetchedBucketListIndexOidAlreadyPresentZeroOid);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              ValidateFetchedBucketListIndexOidMissing);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              CreateBucketListIndexCollisionCount0);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, HandleCollision);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              HandleCollisionMaxAttemptExceeded);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, RegeneratedNewIndexName);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, SaveBucketListIndexOid);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              CreateBucketListIndexFailed);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              CreateBucketListIndexSuccessful);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, ValidateRequestSuceess);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest, ValidateRequestFailed);
};

#endif

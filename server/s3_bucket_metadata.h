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
#include "s3_log.h"

enum class S3BucketMetadataState {
  empty,    // Initial state, no lookup done
  present,  // Metadata exists and was read successfully
  missing,   // Metadata not present in store.
  saved,    // Metadata saved to store.
  deleted,  // Metadata deleted from store
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
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3BucketMetadataState state;

private:
  std::string get_account_index_name() {
    return "ACCOUNT/" + account_name;
  }

  std::string get_account_user_index_name() {
    return "ACCOUNTUSER/" + account_name + "/" + user_name;
  }

  // AWS recommends that all bucket names comply with DNS naming convention
  // See Bucket naming restrictions in above link.
  void validate_bucket_name();

  // Any other validations we want to do on metadata
  void validate();
public:
  S3BucketMetadata(std::shared_ptr<S3RequestObject> req);

  std::string get_bucket_name();
  std::string get_creation_time();
  std::string get_location_constraint();

  void set_location_constraint(std::string location);

  // Load attributes
  void add_system_attribute(std::string key, std::string val);
  void add_user_defined_attribute(std::string key, std::string val);

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

  void remove(std::function<void(void)> on_success, std::function<void(void)> on_failed);

  void remove_account_bucket();
  void remove_account_bucket_successful();
  void remove_account_bucket_failed();
  void remove_user_bucket();
  void remove_user_bucket_successful();
  void remove_user_bucket_failed();


  S3BucketMetadataState get_state() {
    return state;
  }

  // Streaming to json
  std::string to_json();

  void from_json(std::string content);
};

#endif

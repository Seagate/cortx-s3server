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

#ifndef __MERO_FE_S3_SERVER_S3_DELETE_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_DELETE_BUCKET_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_kvs_reader.h"

class S3DeleteBucketAction : public S3Action {
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::map<std::string, std::string>::iterator multipart_kv;
  std::map <std::string, std::string> multipart_objects;
  std::vector<struct m0_uint128> part_oids;

  std::string last_key;  // last key during each iteration

  bool is_bucket_empty;
  bool delete_successful;
  bool multipart_present;

  // Helpers
  std::string get_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name();
  }

  std::string get_multipart_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name() + "/Multipart";
  }

public:
  S3DeleteBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void fetch_bucket_metadata();
  void fetch_first_object_metadata();
  void fetch_first_object_metadata_successful();
  void fetch_first_object_metadata_failed();

  void delete_bucket();
  void delete_bucket_successful();
  void delete_bucket_failed();
  void fetch_multipart_objects();
  void fetch_multipart_objects_successful();
  void remove_part_indexes();
  void remove_part_indexes_successful();
  void remove_part_indexes_failed();
  void remove_multipart_index();

  void send_response_to_s3_client();
};

#endif

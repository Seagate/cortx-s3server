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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Author         :  Rajesh Nambiar        <rajesh.nambiar@seagate.com>
 * Original creation date: 27-Jan-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_GET_MULTIPART_PART_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_GET_MULTIPART_PART_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_object_list_response.h"
#include "s3_clovis_kvs_reader.h"

class S3GetMultipartPartAction : public S3Action {
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata;
  std::string last_key;  // last key during each iteration
  S3ObjectListResponse multipart_part_list;
  size_t return_list_size;
  std:: string bucket_name;
  std::string object_name;
  std::string upload_id;
  m0_uint128 multipart_oid;

  bool fetch_successful;
  bool invalid_upload_id;

  // Request Input params
  std::string request_marker_key;
  size_t max_parts;

  std::string get_part_index_name() {
    return "BUCKET/" + bucket_name + "/" + object_name + "/" + upload_id;
  }

public:
  S3GetMultipartPartAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void get_next_objects();
  void get_next_objects_successful();
  void get_next_objects_failed();
  void get_key_object();
  void get_key_object_successful();
  void get_key_object_failed();
  void fetch_bucket_info();
  void get_multipart_metadata();
  void send_response_to_s3_client();
};

#endif

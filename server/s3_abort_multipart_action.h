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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 6-Jan-2016
 */

#pragma once

#ifndef __S3_SERVER_S3_ABORT_MULTIPART_ACTION_H__
#define __S3_SERVER_S3_ABORT_MULTIPART_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"

class S3AbortMultipartAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata;
  std::shared_ptr<S3PartMetadata> part_metadata;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisWriter> clovis_writer;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::string upload_id;
  std::string bucket_name;
  std::string object_name;
  m0_uint128 multipart_oid;
  m0_uint128 part_index_oid;
  bool abort_success;
  bool invalid_upload_id;

 public:
  S3AbortMultipartAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void fetch_bucket_info();
  void get_multipart_metadata();
  void check_if_any_parts_present();
  void check_if_any_parts_present_failed();
  void delete_multipart_failed();
  void delete_object();
  void delete_object_failed();
  void delete_part_index_with_parts();
  void delete_part_index_with_parts_failed();
  void delete_multipart_metadata();
  void send_response_to_s3_client();
};

#endif

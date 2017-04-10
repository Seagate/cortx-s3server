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
 * Original creation date: 6-Jan-2016
 */

#pragma once

#ifndef __S3_SERVER_S3_POST_COMPLETE_ACTION_H__
#define __S3_SERVER_S3_POST_COMPLETE_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"
#include "s3_uuid.h"

class S3PostCompleteAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ObjectMetadata> multipart_metadata;
  std::shared_ptr<S3PartMetadata> part_metadata;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::shared_ptr<S3ClovisWriter> clovis_writer;
  std::string upload_id;
  std::string bucket_name;
  std::string object_name;
  std::string total_parts;
  std::string etag;
  std::map<std::string, std::string, S3NumStrComparator> parts;
  uint64_t object_size;
  m0_uint128 multipart_index_oid;
  bool delete_multipart_object;

  void parse_xml_str(std::string &xml_str);

 public:
  S3PostCompleteAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void load_and_validate_request();
  void consume_incoming_content();
  bool validate_request_body(std::string &xml_str);
  void fetch_bucket_info();
  void fetch_multipart_info();
  void fetch_multipart_info_failed();

  void fetch_parts_info();
  void get_parts_successful();
  void get_parts_failed();
  void get_part_info(int part);
  void save_metadata();
  void delete_multipart_metadata();
  void delete_multipart_successful();
  void delete_multipart_failed();
  void delete_part_index();
  void delete_part_index_successful();
  void delete_part_index_failed();
  void delete_parts();
  void delete_parts_failed();
  void set_abort_multipart(bool abortit);
  bool is_abort_multipart();
  void delete_old_object_if_present();
  void delete_old_object_failed();
  void send_response_to_s3_client();

  std::string get_part_index_name() {
    return "BUCKET/" + bucket_name + "/" + object_name + "/" + upload_id;
  }

  std::string get_multipart_bucket_index_name() {
    return "BUCKET/" + request->get_bucket_name() + "/Multipart";
  }
};

#endif

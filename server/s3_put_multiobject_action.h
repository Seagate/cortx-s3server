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
 * Original creation date: 20-Jan-2016
 */

#pragma once

#ifndef __S3_SERVER_S3_PUT_MULTIOBJECT_ACTION_H__
#define __S3_SERVER_S3_PUT_MULTIOBJECT_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"
#include "s3_timer.h"

class S3PutMultiObjectAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3PartMetadata> part_metadata;
  std::shared_ptr<S3ObjectMetadata> object_multipart_metadata;
  std::shared_ptr<S3ClovisWriter> clovis_writer;

  size_t total_data_to_stream;
  S3Timer create_object_timer;
  S3Timer write_content_timer;
  int part_number;
  std::string upload_id;

  int get_part_number() {
    return atoi((request->get_query_string_value("partNumber")).c_str());
  }

  bool auth_failed;
  bool write_failed;
  // These 2 flags help respond to client gracefully when either auth or write
  // fails.
  // Both write and chunk auth happen in parallel.
  bool clovis_write_in_progress;
  bool clovis_write_completed;  // full object write
  bool auth_in_progress;
  bool auth_completed;  // all chunk auth

  void chunk_auth_successful();
  void chunk_auth_failed();

 public:
  S3PutMultiObjectAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();
  // void start();

  void fetch_bucket_info();
  void fetch_bucket_info_failed();
  void fetch_multipart_metadata();
  void fetch_multipart_failed();
  void fetch_firstpart_info();
  void fetch_firstpart_info_failed();
  void compute_part_offset();

  void initiate_data_streaming();
  void consume_incoming_content();
  void write_object(S3AsyncBufferContainer& buffer);

  void write_object_successful();
  void write_object_failed();
  void save_metadata();
  void send_response_to_s3_client();
};

#endif

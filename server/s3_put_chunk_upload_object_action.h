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
 * Original creation date: 17-Mar-2016
 */

#pragma once

#ifndef __S3_SERVER_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_H__
#define __S3_SERVER_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_object_metadata.h"
#include "s3_timer.h"

class S3PutChunkUploadObjectAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ClovisWriter> clovis_writer;
  struct m0_uint128 oid;
  // Maximum retry count for collision resolution
  unsigned short tried_count;
  // string used for salting the uri
  std::string salt;
  size_t total_data_to_stream;
  S3Timer create_object_timer;
  S3Timer write_content_timer;

  bool auth_failed;
  bool write_failed;
  // These 2 flags help respond to client gracefully when either auth or write
  // fails.
  // Both write and chunk auth happen in parallel.
  bool clovis_write_in_progress;
  bool clovis_write_completed;  // full object write
  bool auth_in_progress;
  bool auth_completed;  // all chunk auth
  void create_new_oid();
  void collision_detected();
  void send_chunk_details_if_any();

 public:
  S3PutChunkUploadObjectAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void chunk_auth_successful();
  void chunk_auth_failed();

  void fetch_bucket_info();
  void create_object();
  void create_object_failed();

  void initiate_data_streaming();
  void consume_incoming_content();
  void write_object(std::shared_ptr<S3AsyncBufferOptContainer> buffer);

  void write_object_successful();
  void write_object_failed();
  void save_metadata();
  void send_response_to_s3_client();

  // rollback functions
  void rollback_create();
  void rollback_create_failed();
};

#endif

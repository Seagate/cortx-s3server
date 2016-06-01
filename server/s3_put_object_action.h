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

#ifndef __MERO_FE_S3_SERVER_S3_PUT_OBJECT_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_PUT_OBJECT_ACTION_H__

#include <memory>
#include <string>
#include "s3_action_base.h"
#include "s3_async_buffer.h"
#include "s3_bucket_metadata.h"
#include "s3_object_metadata.h"
#include "s3_clovis_writer.h"
#include "s3_timer.h"


class S3PutObjectAction : public S3Action {
  struct m0_uint128 oid;
  // Maximum retry count for collision resolution
  unsigned short tried_count;
  // string used for salting the uri
  std::string salt;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3ClovisWriter> clovis_writer;

  size_t total_data_to_stream;
  S3Timer create_object_timer;
  S3Timer write_content_timer;
  bool write_in_progress;
public:
  S3PutObjectAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();
  // void start();

  void fetch_bucket_info();
  void create_object();
  void create_object_failed();
  void create_new_oid();
  void collision_detected();

  void initiate_data_streaming();
  void consume_incoming_content();
  void write_object(S3AsyncBufferContainer& buffer);

  void write_object_successful();
  void write_object_failed();
  void save_metadata();
  void send_response_to_s3_client();

  // rollback functions
  void rollback_create();
  void rollback_create_failed();
};

#endif

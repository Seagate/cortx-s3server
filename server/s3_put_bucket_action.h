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

#ifndef __S3_SERVER_S3_PUT_BUCKET_ACTION_H__
#define __S3_SERVER_S3_PUT_BUCKET_ACTION_H__

#include <tuple>
#include <vector>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"
#include "s3_put_bucket_body.h"

class S3PutBucketAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory;
  std::shared_ptr<S3PutBucketBodyFactory> put_bucketbody_factory;
  std::shared_ptr<S3PutBucketBody> put_bucket_body;

  std::string request_content;
  std::string location_constraint;  // Received in request body.
 public:
  S3PutBucketAction(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_metadata_factory =
          nullptr,
      std::shared_ptr<S3PutBucketBodyFactory> bucket_body_factory = nullptr);

  void setup_steps();
  void validate_request();
  void consume_incoming_content();
  void validate_request_body(std::string content);
  void read_metadata();
  void create_bucket();
  void create_bucket_failed();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutBucketActionTest, Constructor);
  FRIEND_TEST(S3PutBucketActionTest, ValidateRequest);
  FRIEND_TEST(S3PutBucketActionTest, ValidateRequestInvalid);
  FRIEND_TEST(S3PutBucketActionTest, ValidateRequestMoreContent);
  FRIEND_TEST(S3PutBucketActionTest, ReadMetaData);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketAlreadyExist);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketSuccess);
  FRIEND_TEST(S3PutBucketActionTest, CreateBucketFailed);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientServiceUnavailable);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientMalformedXML);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientNoSuchBucket);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PutBucketActionTest, SendResponseToClientInternalError);
};

#endif

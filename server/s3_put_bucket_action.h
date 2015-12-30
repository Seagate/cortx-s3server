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

#ifndef __MERO_FE_S3_SERVER_S3_PUT_BUCKET_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_PUT_BUCKET_ACTION_H__

#include <vector>
#include <tuple>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"

class S3PutBucketAction : public S3Action {
  std::shared_ptr<S3BucketMetadata> bucket_metadata;

  std::string request_content;
  std::string location_constraint;  // Received in request body.
public:
  S3PutBucketAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();

  void validate_request();

  void consume_incoming_content();
  void validate_request_body(std::string content);

  void read_metadata();
  void create_bucket();
  void send_response_to_s3_client();
};

#endif

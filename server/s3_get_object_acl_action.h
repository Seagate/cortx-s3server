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
 * Original creation date: 20-May-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_GET_OBJECT_ACL_ACTION_H__
#define __MERO_FE_S3_SERVER_S3_GET_OBJECT_ACL_ACTION_H__

#include <memory>

#include "s3_action_base.h"
#include "s3_bucket_metadata.h"

class S3GetObjectACLAction : public S3Action {
  std::shared_ptr<S3ObjectMetadata> object_metadata;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  m0_uint128 object_list_index_oid;

 public:
  S3GetObjectACLAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();
  void fetch_bucket_info();
  void get_object_metadata();
  void send_response_to_s3_client();
};

#endif

/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiarr@seagate.com>
 * Original creation date: 08-Feb-2017
 */

#pragma once

#ifndef __S3_UT_MOCK_S3_BUCKET_METADATA_H__
#define __S3_UT_MOCK_S3_BUCKET_METADATA_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_bucket_metadata.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3BucketMetadata : public S3BucketMetadata {
 public:
  MockS3BucketMetadata(std::shared_ptr<S3RequestObject> req)
      : S3BucketMetadata(req) {}
  MOCK_METHOD2(load, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD0(get_state, S3BucketMetadataState());
  MOCK_METHOD0(get_policy_as_json, std::string &());
  MOCK_METHOD0(get_acl_as_xml, std::string&());
  MOCK_METHOD1(setpolicy, void(std::string& policy_str));
  MOCK_METHOD1(setacl, void(std::string& acl_str));
  MOCK_METHOD2(save, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(remove, void(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed));
  MOCK_METHOD0(deletepolicy, void());
  MOCK_METHOD1(set_location_constraint, void(std::string location));
};

#endif

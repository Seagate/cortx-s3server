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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 10-Nov-2015
 */
#include "s3_server_config.h"
#include <gtest/gtest.h>

// To use a test fixture, derive a class from testing::Test.
class S3ConfigTest : public testing::Test {
  //members protected so that they can be accesses from sub-classes
  protected:

  S3ConfigTest() {
    inst = S3Config::get_instance();
  }

  S3Config *inst;
};

TEST_F(S3ConfigTest, DefaultConstructor) {
  S3Config s3config;
  EXPECT_NE(0, s3config.region_endpoints.size());
  EXPECT_NE(0, s3config.default_endpoint.size());
}

TEST_F(S3ConfigTest, SingletonCheck) {
  S3Config *inst1 = S3Config::get_instance();
  S3Config *inst2 = S3Config::get_instance();
  EXPECT_EQ(inst1, inst2);
  ASSERT_TRUE(inst1);
}

TEST_F(S3ConfigTest, GetDefaultEndPoint) {
  EXPECT_EQ(std::string("s3.seagate.com"), inst->get_default_endpoint());
}

TEST_F(S3ConfigTest, GetRegionEndPoints) {
  std::set<std::string> region_eps = inst->get_region_endpoints();
  EXPECT_TRUE(region_eps.find("s3-asia.seagate.com") != region_eps.end());
  EXPECT_TRUE(region_eps.find("s3-us.seagate.com") != region_eps.end());
  EXPECT_TRUE(region_eps.find("s3-europe.seagate.com") != region_eps.end());
  EXPECT_FALSE(region_eps.find("invalid-region.seagate.com") != region_eps.end());
}

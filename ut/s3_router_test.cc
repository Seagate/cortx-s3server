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
 * Original creation date: 9-Nov-2015
 */

#include "gtest/gtest.h"

#include "s3_router.h"
#include "s3_error_codes.h"

// To use a test fixture, derive a class from testing::Test.
class S3RouterTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3RouterTest() {
    router = new S3Router(NULL, NULL);
  }

  ~S3RouterTest() {
    delete router;
  }

  // A helper functions


  // Declares the variables your tests want to use.
  S3Router *router;
};

TEST_F(S3RouterTest, ReturnsTrueForMatchingDefaultEP) {
  std::string valid_ep = "s3.seagate.com";
  EXPECT_TRUE(router->is_default_endpoint(valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForMisMatchOfDefaultEP) {
  std::string diff_valid_ep = "someregion.s3.seagate.com";
  EXPECT_FALSE(router->is_default_endpoint(diff_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForEmptyDefaultEP) {
  std::string diff_in_valid_ep = "";
  EXPECT_FALSE(router->is_default_endpoint(diff_in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingEP) {
  std::string valid_ep = "s3.seagate.com";
  EXPECT_TRUE(router->is_exact_valid_endpoint(valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingRegionEP) {
  std::string valid_ep = "s3-us.seagate.com";
  EXPECT_TRUE(router->is_exact_valid_endpoint(valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForMisMatchRegionEP) {
  std::string in_valid_ep = "s3-invalid.seagate.com";
  EXPECT_FALSE(router->is_exact_valid_endpoint(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForEmptyRegionEP) {
  std::string in_valid_ep = "";
  EXPECT_FALSE(router->is_exact_valid_endpoint(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingSubEP) {
  std::string valid_ep = "ABC.s3.seagate.com";
  EXPECT_TRUE(router->is_subdomain_match(valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingSubRegionEP) {
  std::string valid_ep = "XYZ.s3-us.seagate.com";
  EXPECT_TRUE(router->is_subdomain_match(valid_ep));
}

TEST_F(S3RouterTest, ReturnsTrueForMatchingEUSubRegionEP) {
  std::string valid_ep = "XYZ.s3-europe.seagate.com";
  EXPECT_TRUE(router->is_subdomain_match(valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForMisMatchSubRegionEP) {
  std::string in_valid_ep = "ABC.s3-invalid.seagate.com";
  EXPECT_FALSE(router->is_subdomain_match(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForInvalidEP) {
  std::string in_valid_ep = "ABC.s3-invalid.google.com";
  EXPECT_FALSE(router->is_subdomain_match(in_valid_ep));
}

TEST_F(S3RouterTest, ReturnsFalseForEmptyEP) {
  std::string in_valid_ep = "";
  EXPECT_FALSE(router->is_subdomain_match(in_valid_ep));
}

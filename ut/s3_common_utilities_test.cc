/*
 * COPYRIGHT 2018 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy  <prashanth.vanaparthy@seagate.com>
 * Original creation date: 21-Aug-2018
 */

#include <gtest/gtest.h>
#include "s3_common_utilities.h"

class S3CommonUtilitiesTest : public testing::Test {};

TEST_F(S3CommonUtilitiesTest, StringHasOnlyDigits) {
  std::string input_string = "123456";
  EXPECT_TRUE(S3CommonUtilities::string_has_only_digits(input_string));
}

TEST_F(S3CommonUtilitiesTest, StringHasChar) {
  std::string input_string = "String";
  EXPECT_FALSE(S3CommonUtilities::string_has_only_digits(input_string));
}

TEST_F(S3CommonUtilitiesTest, StringHasCharAndNumericValues) {
  std::string input_string = "String123";
  EXPECT_FALSE(S3CommonUtilities::string_has_only_digits(input_string));
}

TEST_F(S3CommonUtilitiesTest, StringHasNumericValuesAndSpace) {
  std::string input_string = " 123 ";
  EXPECT_FALSE(S3CommonUtilities::string_has_only_digits(input_string));
}

TEST_F(S3CommonUtilitiesTest, StringHasNumericValuesAndSpecialChars) {
  std::string input_string = "*123?";
  EXPECT_FALSE(S3CommonUtilities::string_has_only_digits(input_string));
}

TEST_F(S3CommonUtilitiesTest, LeftTrimOfStringWithSpaces) {
  std::string input_string = "    teststring";
  EXPECT_STREQ("teststring", S3CommonUtilities::ltrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, LeftTrimOfStringWithTabs) {
  std::string input_string = "\tteststring";
  EXPECT_STREQ("teststring", S3CommonUtilities::ltrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, LeftTrimOfStringWithCarriageReturnAndNewline) {
  std::string input_string = "\r\nteststring";
  EXPECT_STREQ("teststring", S3CommonUtilities::ltrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, LeftTrimOfStringWithNewline) {
  std::string input_string = "\nteststring";
  EXPECT_STREQ("teststring", S3CommonUtilities::ltrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, LeftTrimOfString) {
  std::string input_string = "teststring";
  EXPECT_STREQ("teststring", S3CommonUtilities::ltrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, RightTrimOfStringWithSpaces) {
  std::string input_string = "teststring    ";
  EXPECT_STREQ("teststring", S3CommonUtilities::rtrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, RightTrimOfStringWithTabs) {
  std::string input_string = "teststring\t";
  EXPECT_STREQ("teststring", S3CommonUtilities::rtrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, RightTrimOfStringWithCarriageReturnAndNewline) {
  std::string input_string = "teststring\r\n";
  EXPECT_STREQ("teststring", S3CommonUtilities::rtrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, RightTrimOfStringWithNewline) {
  std::string input_string = "teststring\n";
  EXPECT_STREQ("teststring", S3CommonUtilities::rtrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, RightTrimOfString) {
  std::string input_string = "teststring";
  EXPECT_STREQ("teststring", S3CommonUtilities::rtrim(input_string).c_str());
}

TEST_F(S3CommonUtilitiesTest, TrimOfStringLeftAndRight) {
  std::string input_string = "   test string   ";
  EXPECT_STREQ("test string", S3CommonUtilities::trim(input_string).c_str());
}
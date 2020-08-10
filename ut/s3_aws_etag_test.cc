/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include "s3_aws_etag.h"
#include "gtest/gtest.h"

class S3AwsEtagTest : public testing::Test {
 protected:
  void SetUp() { s3AwsEtag_ptr = new S3AwsEtag(); }

  void TearDown() { delete s3AwsEtag_ptr; }

  S3AwsEtag *s3AwsEtag_ptr;
};

TEST_F(S3AwsEtagTest, Constructor) {
  EXPECT_EQ(0, s3AwsEtag_ptr->part_count);
  EXPECT_EQ("", s3AwsEtag_ptr->hex_etag);
}

TEST_F(S3AwsEtagTest, HexToDec) {
  EXPECT_EQ(0, s3AwsEtag_ptr->hex_to_dec('0'));
  EXPECT_EQ(1, s3AwsEtag_ptr->hex_to_dec('1'));
  EXPECT_EQ(10, s3AwsEtag_ptr->hex_to_dec('A'));
  EXPECT_EQ(11, s3AwsEtag_ptr->hex_to_dec('B'));
  EXPECT_EQ(10, s3AwsEtag_ptr->hex_to_dec('a'));
}

TEST_F(S3AwsEtagTest, HexToDecInvalid) {
  EXPECT_EQ(-1, s3AwsEtag_ptr->hex_to_dec('*'));
}

TEST_F(S3AwsEtagTest, HexToBinary) {
  std::string binary = s3AwsEtag_ptr->convert_hex_bin("abc");
  EXPECT_NE("", binary.c_str());
}

TEST_F(S3AwsEtagTest, AddPartEtag) {
  s3AwsEtag_ptr->hex_etag = "c1d9";
  s3AwsEtag_ptr->add_part_etag("abcd");
  EXPECT_EQ("c1d9abcd", s3AwsEtag_ptr->hex_etag);
  EXPECT_EQ(1, s3AwsEtag_ptr->part_count);
}

TEST_F(S3AwsEtagTest, Finalize) {
  std::string final_etag;
  int part_num_delimiter;
  s3AwsEtag_ptr->hex_etag = "c1d9";
  final_etag = s3AwsEtag_ptr->finalize();
  part_num_delimiter = final_etag.find("-");
  EXPECT_NE(std::string::npos, part_num_delimiter);
  EXPECT_EQ(s3AwsEtag_ptr->part_count,
            atoi(final_etag.substr(part_num_delimiter + 1).c_str()));

  s3AwsEtag_ptr->add_part_etag("abcd");
  final_etag = s3AwsEtag_ptr->finalize();
  part_num_delimiter = final_etag.find("-");
  EXPECT_NE(std::string::npos, part_num_delimiter);
  EXPECT_EQ(s3AwsEtag_ptr->part_count,
            atoi(final_etag.substr(part_num_delimiter + 1).c_str()));
}

TEST_F(S3AwsEtagTest, GetFinalEtag) {
  std::string final_etag;
  s3AwsEtag_ptr->hex_etag = "c1d9";
  final_etag = s3AwsEtag_ptr->finalize();
  EXPECT_NE("", final_etag.c_str());
}

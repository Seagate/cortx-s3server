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

#include "s3_url_encode.h"
#include "gtest/gtest.h"

TEST(S3urlencodeTest, EscapeChar) {
  std::string destination;
  escape_char(' ', destination);
  EXPECT_EQ("%20", destination);
}

TEST(S3urlencodeTest, IsEncodingNeeded) {
  EXPECT_TRUE(char_needs_url_encoding(' '));
  EXPECT_TRUE(char_needs_url_encoding('/'));
  EXPECT_FALSE(char_needs_url_encoding('A'));
}

TEST(S3urlencodeTest, urlEncode) {
  EXPECT_EQ("http%3A%2F%2Ftest%20this", url_encode("http://test this"));
  EXPECT_EQ("abcd", url_encode("abcd"));
}

TEST(S3urlencodeTest, InvalidUrlEncode) {
  EXPECT_EQ("", url_encode(NULL));
  EXPECT_EQ("", url_encode(""));
}

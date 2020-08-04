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

#include "s3_md5_hash.h"
#include "gtest/gtest.h"

TEST(MD5HashTest, Constructor) {
  MD5hash md5hashobj;
  EXPECT_NE(0, md5hashobj.status);
}

TEST(MD5HashTest, Update) {
  char input[] = "abcdefghijklmnopqrstuvwxyz";
  int ret;
  MD5hash md5hashobj;

  ret = md5hashobj.Update(input, sizeof(input));
  EXPECT_EQ(0, ret);

  ret = md5hashobj.Update(NULL, 5);
  EXPECT_NE(0, ret);
}

TEST(MD5HashTest, FinalBasic) {
  char input[] = "abcdefghijklmnopqrstuvwxyz";
  int ret;
  MD5hash md5hashobj;

  md5hashobj.Update(input, 26);
  ret = md5hashobj.Finalize();
  EXPECT_EQ(0, ret);
  // MD5 value from RFC 1321
  // https://www.ietf.org/rfc/rfc1321.txt
  std::string s_hash = md5hashobj.get_md5_string();
  EXPECT_STREQ("c3fcd3d76192e4007dfb496cca67e13b", s_hash.c_str());
}

TEST(MD5HashTest, FinalEmptyStr) {
  char input[] = "";
  int ret;
  MD5hash md5hashobj;
  md5hashobj.Update(input, 0);
  ret = md5hashobj.Finalize();
  EXPECT_EQ(0, ret);
  std::string s_hash = md5hashobj.get_md5_string();
  EXPECT_STREQ("d41d8cd98f00b204e9800998ecf8427e", s_hash.c_str());
}

TEST(MD5HashTest, FinalNumeral) {
  char input[] =
      "123456789012345678901234567890123456789012345678901234567890123456789012"
      "34567890";
  int ret;
  MD5hash md5hashobj;
  md5hashobj.Update(input, 80);
  ret = md5hashobj.Finalize();
  EXPECT_EQ(0, ret);
  std::string s_hash = md5hashobj.get_md5_string();
  EXPECT_STREQ("57edf4a22be3c955ac49da2e2107b67a", s_hash.c_str());
}

TEST(MD5HashTest, GetMD5) {
  char input[] = "abcdefghijklmnopqrstuvwxyz";
  MD5hash md5hashobj;
  md5hashobj.Update(input, 26);
  md5hashobj.Finalize();
  std::string str = md5hashobj.get_md5_string();
  EXPECT_STREQ("c3fcd3d76192e4007dfb496cca67e13b", str.c_str());
}

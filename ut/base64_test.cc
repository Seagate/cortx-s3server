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

#include "gtest/gtest.h"
#include "base64.h"

const unsigned char data[] = {'M', 'a', 'n'};

TEST(Base64, Encode) {
  EXPECT_TRUE(base64_encode(data, 3) == "TWFu");
  EXPECT_TRUE(base64_encode(data, 2) == "TWE=");
  EXPECT_TRUE(base64_encode(data, 1) == "TQ==");
}

TEST(Base64, Decode) {
  EXPECT_TRUE(base64_decode("TWFu") == "Man");
  EXPECT_TRUE(base64_decode("TWE=") == "Ma");
  EXPECT_TRUE(base64_decode("TQ==") == "M");
}

const char encoded_test_text[] =
    "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0"
    "aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1"
    "c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0"
    "aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdl"
    "LCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

const char decoded_test_text[] =
    "Man is distinguished, not only by his reason, but by this singular "
    "passion from other animals, which is a lust of the mind, that by a "
    "perseverance of delight in the continued and indefatigable generation "
    "of knowledge, exceeds the short vehemence of any carnal pleasure.";

TEST(Base64, Gobbs) {
  EXPECT_TRUE(base64_encode((const unsigned char *)decoded_test_text,
                            sizeof(decoded_test_text) - 1) ==
              encoded_test_text);
  EXPECT_TRUE(base64_decode(encoded_test_text) == decoded_test_text);
}

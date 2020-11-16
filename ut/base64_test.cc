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

#include <memory>
#include <string>
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

const char ascii256_encoded[] =
    "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v"
    "MDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f"
    "YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P"
    "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/"
    "wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v"
    "8PHy8/T19vf4+fr7/P3+/w==";

TEST(Base64, Ascii256) {
  enum {
    BUFFER_SIZE = 256
  };
  std::unique_ptr<unsigned char[]> buffer(new unsigned char[BUFFER_SIZE]);

  for (unsigned ch = 0; ch < BUFFER_SIZE; ++ch) {
    buffer[ch] = ch;
  }
  std::string s_encoded = base64_encode(buffer.get(), BUFFER_SIZE);
  const auto encoded_length = s_encoded.length();

  EXPECT_EQ(encoded_length, sizeof(ascii256_encoded) - 1);
  EXPECT_FALSE(memcmp(s_encoded.c_str(), ascii256_encoded, encoded_length));

  std::string s_decoded = base64_decode(s_encoded);

  EXPECT_EQ(s_decoded.length(), BUFFER_SIZE);
  EXPECT_FALSE(memcmp(s_decoded.c_str(), buffer.get(), BUFFER_SIZE));
}

const char ascii512_encoded[] =
    "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v"
    "MDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f"
    "YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P"
    "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/"
    "wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v"
    "8PHy8/T19vf4+fr7/P3+/wABAgMEBQYHCAkKCwwNDg8QERITFBUWFxgZGhscHR4f"
    "ICEiIyQlJicoKSorLC0uLzAxMjM0NTY3ODk6Ozw9Pj9AQUJDREVGR0hJSktMTU5P"
    "UFFSU1RVVldYWVpbXF1eX2BhYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5ent8fX5/"
    "gIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6v"
    "sLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f"
    "4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8=";

TEST(Base64, Ascii512) {
  enum {
    BUFFER_SIZE = 512
  };
  std::unique_ptr<unsigned char[]> buffer(new unsigned char[BUFFER_SIZE]);

  for (unsigned ch = 0; ch < BUFFER_SIZE; ++ch) {
    buffer[ch] = ch;
  }
  std::string s_encoded = base64_encode(buffer.get(), BUFFER_SIZE);
  const auto encoded_length = s_encoded.length();

  EXPECT_EQ(encoded_length, sizeof(ascii512_encoded) - 1);
  EXPECT_FALSE(memcmp(s_encoded.c_str(), ascii512_encoded, encoded_length));

  std::string s_decoded = base64_decode(s_encoded);

  EXPECT_EQ(s_decoded.length(), BUFFER_SIZE);
  EXPECT_FALSE(memcmp(s_decoded.c_str(), buffer.get(), BUFFER_SIZE));
}

const char ascii768_encoded[] =
    "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v"
    "MDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f"
    "YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P"
    "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/"
    "wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v"
    "8PHy8/T19vf4+fr7/P3+/wABAgMEBQYHCAkKCwwNDg8QERITFBUWFxgZGhscHR4f"
    "ICEiIyQlJicoKSorLC0uLzAxMjM0NTY3ODk6Ozw9Pj9AQUJDREVGR0hJSktMTU5P"
    "UFFSU1RVVldYWVpbXF1eX2BhYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5ent8fX5/"
    "gIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6v"
    "sLGys7S1tre4ubq7vL2+v8DBwsPExcbHyMnKy8zNzs/Q0dLT1NXW19jZ2tvc3d7f"
    "4OHi4+Tl5ufo6err7O3u7/Dx8vP09fb3+Pn6+/z9/v8AAQIDBAUGBwgJCgsMDQ4P"
    "EBESExQVFhcYGRobHB0eHyAhIiMkJSYnKCkqKywtLi8wMTIzNDU2Nzg5Ojs8PT4/"
    "QEFCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaW1xdXl9gYWJjZGVmZ2hpamtsbW5v"
    "cHFyc3R1dnd4eXp7fH1+f4CBgoOEhYaHiImKi4yNjo+QkZKTlJWWl5iZmpucnZ6f"
    "oKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr/AwcLDxMXGx8jJysvMzc7P"
    "0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn6Onq6+zt7u/w8fLz9PX29/j5+vv8/f7/";

TEST(Base64, Ascii768) {
  enum {
    BUFFER_SIZE = 768
  };
  std::unique_ptr<unsigned char[]> buffer(new unsigned char[BUFFER_SIZE]);

  for (unsigned ch = 0; ch < BUFFER_SIZE; ++ch) {
    buffer[ch] = ch;
  }
  std::string s_encoded = base64_encode(buffer.get(), BUFFER_SIZE);
  const auto encoded_length = s_encoded.length();

  EXPECT_EQ(encoded_length, sizeof(ascii768_encoded) - 1);
  EXPECT_FALSE(memcmp(s_encoded.c_str(), ascii768_encoded, encoded_length));

  std::string s_decoded = base64_decode(s_encoded);

  EXPECT_EQ(s_decoded.length(), BUFFER_SIZE);
  EXPECT_FALSE(memcmp(s_decoded.c_str(), buffer.get(), BUFFER_SIZE));
}

/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "base62.h"

using ::testing::SizeIs;

using namespace base62;

const std::string kBase62 =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

// Tests the encoding of values 0-61.
TEST(Base62, SimpleEncode) {
  for (uint64_t i = 0; i < 62; i++) {
    EXPECT_EQ(base62_encode(i), std::string{kBase62.at(i)});
  }
}

// Tests the decoding of single character strings [0-9A-Za-z].
TEST(Base62, SimpleDecode) {
  for (size_t i = 0; i < 62; i++) {
    EXPECT_EQ(base62_decode(std::string{kBase62[i]}), i);
  }
}

// Tests the encoding of minimum and maximum integer values.
TEST(Base62, EncodeMinMax) {
  EXPECT_EQ(base62_encode(0), "0");
  EXPECT_EQ(base62_encode(static_cast<uint64_t>(std::pow(62, 8)) - 1),
            "zzzzzzzz");
  EXPECT_EQ(base62_encode(UINT64_MAX), "LygHa16AHYF");
}

// Tests the decoding of minimum and maximum encoded strings.
TEST(Base62, DecodeMinMax) {
  EXPECT_EQ(base62_decode("0"), 0);
  EXPECT_EQ(base62_decode("zzzzzzzz"),
            static_cast<uint64_t>(std::pow(62, 8)) - 1);
  EXPECT_EQ(base62_decode("LygHa16AHYF"), UINT64_MAX);
}

// Tests that encoded strings are padded.
TEST(Base62, EncodeWithPadding) {
  EXPECT_EQ(base62_encode(63, /*pad*/ 0), "11");
  EXPECT_EQ(base62_encode(0, /*pad*/ 5), "00000");
  EXPECT_EQ(base62_encode(61, /*pad*/ 11), "0000000000z");
  EXPECT_EQ(base62_encode(UINT64_MAX, /*pad*/ 1), "LygHa16AHYF");
}

// Tests the decoding of strings that have leading zeroes.
TEST(Base62, DecodeLeadingZeroes) {
  EXPECT_EQ(base62_decode("0"), 0);
  EXPECT_EQ(base62_decode("000000"), 0);
  EXPECT_EQ(base62_decode("00000000000"), 0);

  EXPECT_EQ(base62_decode("0A"), 10);
  EXPECT_EQ(base62_decode("000A"), 10);
  EXPECT_EQ(base62_decode("0000000000A"), 10);

  EXPECT_EQ(base62_decode("Sr3dQSZ"), 1639015361963);
  EXPECT_EQ(base62_decode("0Sr3dQSZ"), 1639015361963);
  EXPECT_EQ(base62_decode("0000Sr3dQSZ"), 1639015361963);
}

// Tests the behavior of decoding strings that are too large.
TEST(Base62, Overflows) {
  EXPECT_EQ(base62_decode("LygHa16AHYG"), 0);   // UINT64_MAX + 1
  EXPECT_EQ(base62_decode("LygHa16AHZG"), 62);  // UINT64_MAX + 62
  EXPECT_EQ(base62_decode("zzzzzzzzzzz"),
            UINT64_C(15143072536417990655));  // (62^11 - 1) & UINT64_MAX
}

// Test the encoding and decoding of a random number of values, and ensures they
// are in sorted ordering.
TEST(Base62, RandomRange) {
  // There are far too many values between 0 and UINT64_MAX
  // to test them all, so pick a set of random ones to vary
  // the encoding and decoding.

  // We don't need a cryptographically secure seed, but repeatable is nice
  auto seed = std::chrono::system_clock::now().time_since_epoch().count();
  ::testing::Test::RecordProperty("Random Seed", std::to_string(seed));
  std::cout << "Random Seed: " << seed << "\n";

  std::mt19937_64 rng{seed};

  std::vector<uint64_t> values(5000);
  for (auto& v : values) v = rng();

  // Sort random numbers to check that the encodings are also in a sorted
  // ordering
  std::sort(values.begin(), values.end());

  std::vector<std::string> encodings;
  for (auto val : values) {
    // For sorting purposes, all encodings must be the same length
    auto encoded = base62_encode(val, /*pad*/ 11);
    EXPECT_THAT(encoded, SizeIs(11));
    EXPECT_EQ(base62_decode(encoded), val);
    encodings.push_back(std::move(encoded));
  }
  EXPECT_TRUE(std::is_sorted(encodings.begin(), encodings.end()));
}

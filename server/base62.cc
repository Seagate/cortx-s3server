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

#include <array>
#include <cassert>
#include <string>
#include "base62.h"

// This is the maximum string length of an encoded uint64_t, for
// UINT64_MAX. Still, an 11-character string can overflow uint64_t.
// The string "LygHa16AHYF" is the encoded value of UINT64_MAX.
// The max 11-character string "zzzzzzzzzzz" is a 66-bit integer.
constexpr size_t kMaxEncodedLen = 11;

// Integer to Base62 encoding table. Characters are sorted in
// lexicographical order, which makes the encoded result
// also sortable in the same way as the integer source.
constexpr std::array<char, 62> kBase62CharSet{
    // 0-9
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    // A-Z
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    // a-z
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

// Decodes a single Base62 letter into its integer equivalent.
static uint64_t letter_to_remainder(char c) {
  if (c >= '0' && c <= '9') return c % '0';
  if (c >= 'A' && c <= 'Z') return (c % 'A') + 10;
  if (c >= 'a' && c <= 'z') return (c % 'a') + 36;
  assert(false && "Invalid Base62 character");
  return -1;
}

namespace base62 {

std::string base62_encode(uint64_t value, size_t pad) {
  std::string ret;

  if (value == 0) {
    ret = kBase62CharSet[0];
  }

  while (value > 0) {
    ret = kBase62CharSet[value % kBase62CharSet.size()] + ret;
    value /= kBase62CharSet.size();
  }

  if (ret.size() < pad) ret.insert(0, pad - ret.size(), kBase62CharSet[0]);

  return ret;
}

uint64_t base62_decode(const std::string& value) {
  // Fail obviously overflowing values, but this does not
  // account for all possibilities. Values greater than
  // "LygHa16AHYF" will just overflow.
  assert((value.size() <= kMaxEncodedLen) && "Base62 overflow");
  if (value.size() > kMaxEncodedLen) {
    return -1;
  }

  uint64_t mult = 1;
  uint64_t result = 0;
  for (auto cit = value.rbegin(); cit != value.rend(); ++cit) {
    auto rem = letter_to_remainder(*cit);
    result += rem * mult;
    mult *= kBase62CharSet.size();
  }

  return result;
}

}  // namespace base62
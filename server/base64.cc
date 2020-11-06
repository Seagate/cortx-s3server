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

#include <cassert>
#include <cctype>
#include <iostream>
#include "base64.h"

const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

const signed char decode_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};

std::string base64_encode(unsigned char const* pb, unsigned in_len) {

  std::string ret;
  ret.reserve((in_len + 2) / 3 << 2);

  unsigned modulo_3 = 0;
  int prev_ch = 0;

  for (unsigned i = 0; i < in_len; ++i) {
    const int cur_ch = *(pb + i);

    switch (modulo_3) {
      case 0:
        ret += base64_chars[cur_ch >> 2 & 0x3F];
        break;
      case 1:
        ret += base64_chars[(prev_ch << 4 & 0x30) | (cur_ch >> 4 & 0xF)];
        break;
      case 2:
        ret += base64_chars[(prev_ch << 2 & 0x3C) | (cur_ch >> 6 & 3)];
        ret += base64_chars[cur_ch & 0x3F];
        break;
      default:
        assert(0);
    }
    if (++modulo_3 > 2) {
      modulo_3 = 0;
    }
    prev_ch = cur_ch;
  }
  if (1 == modulo_3) {
    ret += base64_chars[prev_ch << 4 & 0x30];
    ret += "==";
  } else if (2 == modulo_3) {
    ret += base64_chars[prev_ch << 2 & 0x3C];
    ret += '=';
  }
  return ret;
}

std::string base64_decode(const std::string& encoded_string) {

  std::string ret;
  ret.reserve(((encoded_string.length() + 3) >> 2) * 3);  // roughly

  unsigned modulo_4 = 0;
  int current_byte = 0;  // -Wall

  for (const char ch : encoded_string) {

    if (isspace(ch)) {
      if (modulo_4) {
        break;  // Bad input
      } else {
        continue;  // base64 text can be formatted
      }
    }
    const int decoded = decode_table[ch & 255];
    if (decoded < 0) break;

    switch (modulo_4) {
      case 0:
        current_byte = decoded << 2;
        break;
      case 1:
        ret += static_cast<char>(current_byte | (decoded >> 4 & 3));
        current_byte = decoded << 4;
        break;
      case 2:
        ret += static_cast<char>(current_byte | (decoded >> 2 & 0x0F));
        current_byte = decoded << 6;
        break;
      case 3:
        ret += static_cast<char>(current_byte | (decoded & 0x3F));
        break;
      default:
        assert(0);
    }
    if (++modulo_4 > 3) modulo_4 = 0;
  }
  return ret;
}

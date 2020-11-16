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

std::string base64_encode(unsigned char const* bytes_to_encode,
                          unsigned in_len) {

  std::string ret;
  ret.reserve((in_len + 2) / 3 << 2);  // exact size

  unsigned modulo_3 = 0;
  int for_next = 0;  // -Wall

  for (unsigned i = 0; i < in_len; ++i) {
    const int cur_ch = *(bytes_to_encode + i);

    switch (modulo_3) {
      case 0:
        ret += base64_chars[cur_ch >> 2 & 0x3F];
        for_next = (cur_ch & 3) << 4;
        break;
      case 1:
        assert(!(for_next & ~0x30));
        ret += base64_chars[for_next | (cur_ch >> 4 & 0xF)];
        for_next = (cur_ch & 0x0F) << 2;
        break;
      case 2:
        assert(!(for_next & ~0x3C));
        ret += base64_chars[for_next | (cur_ch >> 6 & 3)];
        ret += base64_chars[cur_ch & 0x3F];
        break;
      default:
        assert(0);
    }
    if (++modulo_3 > 2) {
      modulo_3 = 0;
    }
  }
  assert(modulo_3 < 3);

  if (modulo_3) {
    assert(!(for_next & ~0x3F));
    ret += base64_chars[for_next];
    ret += '=';

    if (1 == modulo_3) {
      ret += '=';
    }
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
    int decoded = 0;

    if (ch >= 'A' && ch <= 'Z')
      decoded = ch - 'A';
    else if (ch >= 'a' && ch <= 'z')
      decoded = ch - ('a' - 26);
    else if (ch >= '0' && ch <= '9')
      decoded = ch + (52 - '0');
    else if ('+' == ch)
      decoded = 62;
    else if ('/' == ch)
      decoded = 63;
    else {
      // Illegal base64 character
      break;
    }
    switch (modulo_4) {
      case 0:
        current_byte = decoded << 2;
        break;
      case 1:
        assert(!(current_byte & 3));
        ret += static_cast<char>(current_byte | (decoded >> 4 & 3));
        current_byte = decoded << 4;
        break;
      case 2:
        assert(!(current_byte & 0x0F));
        ret += static_cast<char>(current_byte | (decoded >> 2 & 0x0F));
        current_byte = decoded << 6;
        break;
      case 3:
        assert(!(current_byte & 0x3F));
        ret += static_cast<char>(current_byte | (decoded & 0x3F));
        break;
      default:
        assert(0);
    }
    if (++modulo_4 > 3) modulo_4 = 0;
  }
  return ret;
}

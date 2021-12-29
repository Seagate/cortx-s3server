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

#pragma once

#ifndef __S3_SERVER_BASE62_H__
#define __S3_SERVER_BASE62_H__

#include <cstdint>
#include <string>

namespace base62 {

// Returns the lexicographically-sorted Base62 string encoding of an
// unsigned 64-bit integer. If `pad` is specified, the resulting string
// will be padded with the "zero" Base62 characters, which preserves sorting.
std::string base62_encode(uint64_t value, size_t pad = 0);

// Returns the 64-bit unsigned integer decoded from a lexicographically-sorted
// Base62 string.
uint64_t base62_decode(std::string const& value);

}  // namespace base62

#endif

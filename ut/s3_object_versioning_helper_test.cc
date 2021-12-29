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
#include <functional>
#include <map>
#include <regex>
#include <string>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "s3_object_versioning_helper.h"

using ::testing::ElementsAreArray;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::MatchesRegex;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::WhenSortedBy;

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Millis = std::chrono::milliseconds;
using UnsignedMillis = std::chrono::duration<uint64_t, std::milli>;

constexpr uint64_t kMaxTimeStampCount = 218340105584895;  // 62^8 - 1

const auto kVersionIdRegExMatcher = MatchesRegex(
    "^"                   // start of string
    "[0-9A-Za-z]{8}"      // base62 timestamp
    "[0-9A-Za-z_-]{22,}"  // url-safe base64, 22 or more chars
    "$"                   // end of string
    );

// Splits a version ID, returning the timestamp component as the first
// item, and the random UUID as the second item.
std::pair<std::string, std::string> SplitVersionId(
    const std::string& version_id) {
  return std::make_pair(std::string(version_id, 0, 8),
                        std::string(version_id, 9, version_id.length()));
}

// Tests that version ID timestamps are generated correctly from known inputs.
TEST(ObjectVersioningHelper, GenerateTimestampFromKnownTimePoints) {
  // Thursday, 01 January 1970 00:00:00.000 GMT
  EXPECT_EQ(S3ObjectVersioningHelper::generate_timestamp(TimePoint{}),
            kMaxTimeStampCount);

  // Unix time 1639463595165 -> Tuesday, 14 December 2021 06:33:15.165 GMT
  EXPECT_EQ(S3ObjectVersioningHelper::generate_timestamp(
                TimePoint{Millis{1639463595165}}),
            216700641989730u);

  // With GCC, the system clock has a nanosecond precision, and so the full
  // range of time is:
  //   [1677-09-21 00:12:43.145224192, 2262-04-11 23:47:16.854775807]
  // The maximum epoch time is 9223372036854. Thus, it is not yet possible to
  // represent the maximum
  // version ID timestamp of 218340105584895, which is Thursday, 02 December
  // 8888 13:19:44.895 GMT.
  EXPECT_EQ(S3ObjectVersioningHelper::generate_timestamp(TimePoint::max()),
            209116733548041u);

  const auto max_time_ms =
      std::chrono::time_point_cast<UnsignedMillis>(TimePoint::max())
          .time_since_epoch()
          .count();
  EXPECT_GT(kMaxTimeStampCount, max_time_ms);
}

// Tests that version ID timestamps are generated in reverse time order,
// i.e. increasing unit times produce decreasing version ID timestamps.
TEST(ObjectVersioningHelper, GenerateTimestampFromTimePoints) {
  auto now = Clock::now();
  std::vector<uint64_t> timestamps(500);

  for (auto& timestamp : timestamps) {
    now += Millis{1};
    timestamp = S3ObjectVersioningHelper::generate_timestamp(now);
  }

  // Timestamps should be in the reverse order of actual time
  EXPECT_TRUE(std::is_sorted(timestamps.begin(), timestamps.end(),
                             std::greater<uint64_t>()));
}

// Tests that version IDs are generated correctly from known inputs.
TEST(ObjectVersioningHelper, GenerateVersionIdFromKnownTimestamps) {
  // run the same tests multiple times, in order to vary
  // the random data component of the ID.
  for (int i = 0; i < 100; i++) {
    std::vector<std::string> results;
    std::string version_id;

    // Unix time 218340105584895 -> Thursday, 02 December 8888 13:19:44.895
    // GMT
    version_id = S3ObjectVersioningHelper::get_versionid_from_timestamp(0);
    EXPECT_THAT(version_id, StartsWith("00000000"));
    EXPECT_THAT(version_id, SizeIs(Gt(29)));
    EXPECT_THAT(version_id, kVersionIdRegExMatcher);
    results.push_back(version_id);

    // Unix time 9223372036854 (system_clock max) -> Friday, 11 April 2262
    // 23:47:16.854 GMT
    version_id =
        S3ObjectVersioningHelper::get_versionid_from_timestamp(209116733548041);
    EXPECT_THAT(version_id, StartsWith("xNcH8szR"));
    EXPECT_THAT(version_id, SizeIs(Gt(29)));
    EXPECT_THAT(version_id, kVersionIdRegExMatcher);
    results.push_back(version_id);

    // Unix time 1639463595165 -> Tuesday, 14 December 2021 06:33:15.165 GMT
    version_id =
        S3ObjectVersioningHelper::get_versionid_from_timestamp(216700641989730);
    EXPECT_THAT(version_id, StartsWith("zX8S1pbe"));
    EXPECT_THAT(version_id, SizeIs(Gt(29)));
    EXPECT_THAT(version_id, kVersionIdRegExMatcher);
    results.push_back(version_id);

    // This is Unix time 0 -> Thursday, 01 January 1970 00:00:00.000 GMT
    version_id = S3ObjectVersioningHelper::get_versionid_from_timestamp(
        kMaxTimeStampCount);
    EXPECT_THAT(version_id, StartsWith("zzzzzzzz"));
    EXPECT_THAT(version_id, SizeIs(Gt(29)));
    EXPECT_THAT(version_id, kVersionIdRegExMatcher);
    results.push_back(version_id);

    // IDs are already lexicographically sorted because we created
    // them in order new -> old
    EXPECT_TRUE(std::is_sorted(results.begin(), results.end()));
  }
}

// Tests that version IDs created with the same timestamp are
// unique, but also have the same timestamp.
TEST(ObjectVersioningHelper, SameTime) {
  std::set<std::string> timestamps;
  std::set<std::string> rand;

  const auto ts_now =
      S3ObjectVersioningHelper::generate_timestamp(Clock::now());
  const size_t count{10};

  for (size_t i = 0; i < count; i++) {
    const auto version_id =
        S3ObjectVersioningHelper::get_versionid_from_timestamp(ts_now);
    EXPECT_THAT(version_id, kVersionIdRegExMatcher);

    auto split_id = SplitVersionId(version_id);
    timestamps.insert(split_id.first);
    rand.insert(split_id.second);
  }

  EXPECT_THAT(timestamps, SizeIs(1));
  EXPECT_THAT(rand, SizeIs(count));
}

TEST(ObjectVersioningHelper, NearlySameTime) {
  std::vector<std::string> versions;
  versions.reserve(1048576);

  const auto start = Clock::now();
  const auto stop = start + Millis{500};

  // Try to generate a bunch of IDs at the same or nearly same time.
  for (;;) {
    const auto now = Clock::now();
    versions.push_back(S3ObjectVersioningHelper::get_versionid_from_timestamp(
        S3ObjectVersioningHelper::generate_timestamp(now)));

    if (now > stop) {
      break;
    }
  }

  // Confirm version ID formats, and split the timestamp
  // and random components from the IDs.
  std::vector<std::string> timestamps;
  std::set<std::string> rand;
  for (const auto& version : versions) {
    EXPECT_THAT(version, kVersionIdRegExMatcher);

    auto split_id = SplitVersionId(version);
    timestamps.push_back(split_id.first);
    rand.insert(split_id.second);
  }

  // Version ID timestamps should be in the reverse order of actual time
  EXPECT_TRUE(std::is_sorted(timestamps.begin(), timestamps.end(),
                             std::greater<std::string>()));

  // Expect some duplicate timestamps, but not all. Rationale:
  //  1) It will take longer than 1 millisecond to generate all version IDs
  //  2) More than one version can be generated each millisecond
  std::set<std::string> unique_ts(timestamps.begin(), timestamps.end());
  EXPECT_THAT(unique_ts, SizeIs(Gt(1)));
  EXPECT_THAT(unique_ts, SizeIs(Lt(timestamps.size())));

  EXPECT_THAT(rand, SizeIs(versions.size()));
}

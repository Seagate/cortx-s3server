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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "s3_http_post_queue_impl.h"

using ::testing::_;
using ::testing::DefaultValue;
using ::testing::Return;

class MockS3HttpPostEngine : public S3HttpPostEngine {
 public:
  ~MockS3HttpPostEngine();

  MOCK_METHOD2(set_callbacks,
               bool(std::function<void(void)>, std::function<void(void)>));
  MOCK_METHOD1(post, bool(const std::string &));
};

MockS3HttpPostEngine::~MockS3HttpPostEngine() = default;

class S3HttpPostQueueTest : public testing::Test {
 protected:
  S3HttpPostQueueTest();
  void SetUp() override;

  std::unique_ptr<S3HttpPostQueueImpl> object;
};

S3HttpPostQueueTest::S3HttpPostQueueTest() = default;

void S3HttpPostQueueTest::SetUp() {
  auto *engine = new MockS3HttpPostEngine();

  EXPECT_CALL(*engine, set_callbacks(_, _)).Times(1);
  EXPECT_CALL(*engine, post(_)).WillRepeatedly(Return(true));

  object.reset(new S3HttpPostQueueImpl(engine));
}

const char msg[] = "{\"k\": \"v\"}";

TEST_F(S3HttpPostQueueTest, Basic) {
  EXPECT_FALSE(object->post(""));

  EXPECT_TRUE(object->post(msg));
  EXPECT_EQ(1, object->msg_queue.size());

  object->on_msg_sent();
  EXPECT_TRUE(object->msg_queue.empty());
}

TEST_F(S3HttpPostQueueTest, InProgress) {
  EXPECT_TRUE(object->post(msg));
  EXPECT_TRUE(object->msg_in_progress);

  object->on_error();
  EXPECT_TRUE(object->msg_in_progress);

  object->on_msg_sent();
  EXPECT_FALSE(object->msg_in_progress);
}

TEST_F(S3HttpPostQueueTest, ErrorCount) {
  EXPECT_TRUE(object->post(msg));

  for (unsigned i = 0; i < S3HttpPostQueueImpl::MAX_ERR;) {
    object->on_error();
    EXPECT_EQ(++i, object->n_err);
  }
  object->on_msg_sent();
  EXPECT_EQ(0, object->n_err);
}

TEST_F(S3HttpPostQueueTest, Thresholds) {
  object->n_err = S3HttpPostQueueImpl::MAX_ERR;
  EXPECT_FALSE(object->post(msg));

  --object->n_err;
  EXPECT_TRUE(object->post(msg));

  object->on_msg_sent();

  for (unsigned i = 0; i < S3HttpPostQueueImpl::MAX_MSG_IN_QUEUE - 1; ++i) {
    object->msg_queue.push(std::string());
  }
  EXPECT_TRUE(object->post(msg));
  EXPECT_FALSE(object->post(msg));
}

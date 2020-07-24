/*
 * COPYRIGHT 2020 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Evgeniy Brazhnikov   <evgeniy.brazhnikov@seagate.com>
 * Original creation date: 24-Jul-2020
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

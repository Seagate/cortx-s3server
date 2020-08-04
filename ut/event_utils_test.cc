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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_event_wrapper.h"
#include "event_utils.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

// helper class, defines action callback, and calls counter.
class EventTest : public RecurringEventBase {
 public:
  int calls_count = 0;
  virtual void action_callback(void) noexcept { ++calls_count; }
  using RecurringEventBase::RecurringEventBase;  // inherit constructor
};

class RecurringEventBaseTest : public testing::Test {
 protected:
  EventTest *event_test;
  evbase_t *evbase;
  std::shared_ptr<MockEventWrapper> mock_event_obj_ptr;
  std::shared_ptr<EventWrapper> real_event_obj_ptr;

 public:
  virtual void SetUp() {
    evbase = event_base_new();
    mock_event_obj_ptr = std::make_shared<MockEventWrapper>();
    real_event_obj_ptr = std::make_shared<EventWrapper>();
    event_test = new EventTest(mock_event_obj_ptr, evbase);
  }
  void TearDown() {
    if (event_test) {
      delete event_test;
      event_test = nullptr;
    }
    if (evbase) {
      event_base_free(evbase);
      evbase = nullptr;
    }
    mock_event_obj_ptr.reset();
    real_event_obj_ptr.reset();
  }
};

TEST_F(RecurringEventBaseTest, TestEventCallback) {
  // make sure it does not crash with NULL:
  RecurringEventBase::libevent_callback(0, 0, nullptr);

  // make sure it calls the callback:
  RecurringEventBase::libevent_callback(
      0, 0, static_cast<void *>(static_cast<RecurringEventBase *>(event_test)));
  EXPECT_EQ(event_test->calls_count, 1);
}

TEST_F(RecurringEventBaseTest, TestAddEvtimerSuccess) {
  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

  // Check that it registers an event
  EXPECT_CALL(*mock_event_obj_ptr, new_event(_, _, _, _, _))
      .Times(1)  // dummy comment to trick clang-format
      .WillOnce(Invoke(real_event_obj_ptr.get(), &EventWrapper::new_event));
  EXPECT_CALL(*mock_event_obj_ptr, add_event(_, _)).Times(1);
  int rc = event_test->add_evtimer(tv);
  EXPECT_EQ(0, rc);

  // Check that deregister does what expected
  EXPECT_CALL(*mock_event_obj_ptr, del_event(_)).Times(1);
  EXPECT_CALL(*mock_event_obj_ptr, free_event(_))
      .Times(1)  // dummy comment to trick clang-format
      .WillOnce(Invoke(real_event_obj_ptr.get(), &EventWrapper::free_event));
  event_test->del_evtimer();
}

TEST_F(RecurringEventBaseTest, TestDestructor) {
  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

  // Check that it deregisters an event in destructor
  EXPECT_CALL(*mock_event_obj_ptr, new_event(_, _, _, _, _))
      .Times(1)  // dummy comment to trick clang-format
      .WillOnce(Invoke(real_event_obj_ptr.get(), &EventWrapper::new_event));
  EXPECT_CALL(*mock_event_obj_ptr, add_event(_, _)).Times(1);
  int rc = event_test->add_evtimer(tv);
  EXPECT_EQ(0, rc);

  EXPECT_CALL(*mock_event_obj_ptr, del_event(_)).Times(1);
  EXPECT_CALL(*mock_event_obj_ptr, free_event(_))
      .Times(1)  // dummy comment to trick clang-format
      .WillOnce(Invoke(real_event_obj_ptr.get(), &EventWrapper::free_event));
  delete event_test;
  event_test = nullptr;
}

TEST_F(RecurringEventBaseTest, TestNewEventFailure) {
  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

  // Check that it handles a failure of event memory allocation.
  EXPECT_CALL(*mock_event_obj_ptr, new_event(_, _, _, _, _))
      .Times(1)  // dummy comment to trick clang-format
      .WillOnce(Return(nullptr));
  // won't be called as alloc failed
  EXPECT_CALL(*mock_event_obj_ptr, add_event(_, _)).Times(0);
  int rc = event_test->add_evtimer(tv);
  EXPECT_EQ(-ENOMEM, rc);

  // Make sure no deallocation attempt is done
  EXPECT_CALL(*mock_event_obj_ptr, del_event(_)).Times(0);
  EXPECT_CALL(*mock_event_obj_ptr, free_event(_)).Times(0);
  delete event_test;
  event_test = nullptr;
}

TEST_F(RecurringEventBaseTest, TestAddEvtimerHandleNull) {
  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
  int rc;

  EventTest ev1(nullptr, evbase);
  rc = ev1.add_evtimer(tv);
  EXPECT_EQ(-EINVAL, rc);

  EventTest ev2(nullptr, nullptr);
  rc = ev2.add_evtimer(tv);
  EXPECT_EQ(-EINVAL, rc);

  EventTest ev3(mock_event_obj_ptr, nullptr);
  rc = ev3.add_evtimer(tv);
  EXPECT_EQ(-EINVAL, rc);
}

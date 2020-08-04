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

#pragma once

#ifndef __S3_SERVER_EVENT_UTILS_H__
#define __S3_SERVER_EVENT_UTILS_H__

#include <memory>

#include <gtest/gtest_prod.h>

#include "s3_common.h"
#include "event_wrapper.h"
#include "evhtp_wrapper.h"

// Allows to create recurring events using libevent. I.e. to schedule certain
// action to happen regularly at specified interval.
//
// Usage:
//
// // Define:
// class MyClass : public RecurringEventBase {
//  public:
//   virtual void action_callback(void) noexcept {
//     // do my stuff here
//   }
// };
//
// // Instantiate & Register:
// MyClass evt(std::make_shared<EventWrapper>(),
//             S3Option::get_instance()->get_eventbase());
// struct timeval tv;
// tv.tv_sec = 1;
// tv.tv_usec = 0;
// evt.add_evtimer(tv);
//
// Now "my stuff" will be issued every 1 second.
// Note that destructor deregisters the event, so MyClass instance must be
// preserved for the entire duration of the period when action recurrence is
// needed.
class RecurringEventBase {
 private:
  std::shared_ptr<EventInterface> event_obj;
  struct event *evt = nullptr;
  evbase_t *evbase = nullptr;

 protected:
  static void libevent_callback(evutil_socket_t fd, short event, void *arg);

 public:
  RecurringEventBase(std::shared_ptr<EventInterface> event_obj_ptr,
                     evbase_t *evbase_ = nullptr)
      : event_obj(std::move(event_obj_ptr)), evbase(evbase_) {}
  virtual ~RecurringEventBase() { del_evtimer(); }

  // Registers the event with libevent, recurrence specified by 'tv'.
  int add_evtimer(struct timeval &tv);
  // Can be used to deregister an event (can be re-registered later with same
  // or different recurrence interval, using add_evtimer);
  void del_evtimer();
  // This callback will be called on every occurrence of the event.
  virtual void action_callback(void) noexcept = 0;

  FRIEND_TEST(RecurringEventBaseTest, TestEventCallback);
};

#endif

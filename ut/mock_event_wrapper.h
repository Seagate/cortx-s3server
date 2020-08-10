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

#ifndef __S3_UT_MOCK_EVENT_WRAPPER_H__
#define __S3_UT_MOCK_EVENT_WRAPPER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "event_wrapper.h"

class MockEventWrapper : public EventInterface {
 public:
  MockEventWrapper() {}
  MOCK_METHOD5(new_event,
               struct event *(struct event_base *base, evutil_socket_t fd,
                              short events,
                              void (*cb)(evutil_socket_t, short, void *),
                              void *arg));
  MOCK_METHOD1(del_event, void(struct event *s3_client_event));
  MOCK_METHOD1(free_event, void(struct event *s3_client_event));
  MOCK_METHOD2(add_event,
               void(struct event *s3_client_event, struct timeval *tv));
  MOCK_METHOD3(pending_event, bool(struct event *s3_client_event, short event,
                                   struct timeval *tv));
};

#endif

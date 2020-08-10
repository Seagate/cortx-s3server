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

#include "event_wrapper.h"

//++
// Note: Its being observed that when wrapper name is same as that
// of actual event api in libevent "C" library, then s3server goes
// to hung state.
//--

struct event *EventWrapper::new_event(
    struct event_base *base, evutil_socket_t fd, short events,
    void (*callback)(evutil_socket_t, short, void *), void *arg) {
  return event_new(base, fd, events, callback, arg);
}

void EventWrapper::del_event(struct event *s3_client_event) {
  event_del(s3_client_event);
}

void EventWrapper::free_event(struct event *s3_client_event) {
  event_free(s3_client_event);
}

bool EventWrapper::pending_event(struct event *s3_client_event, short event,
                                 struct timeval *tv) {
  return event_pending(s3_client_event, event, tv);
}

void EventWrapper::add_event(struct event *s3_client_event,
                             struct timeval *tv) {
  event_add(s3_client_event, tv);
}

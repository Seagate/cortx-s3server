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

#include "event_utils.h"
#include "s3_log.h"

void RecurringEventBase::libevent_callback(evutil_socket_t fd, short event,
                                           void *arg) {
  s3_log(S3_LOG_DEBUG, "", "Entering");
  RecurringEventBase *evt_base = static_cast<RecurringEventBase *>(arg);
  if (evt_base) {
    evt_base->action_callback();
  }
}

int RecurringEventBase::add_evtimer(struct timeval &tv) {
  s3_log(S3_LOG_DEBUG, "", "Entering with sec=%" PRIu32 " usec=%" PRIu32,
         (uint32_t)tv.tv_sec, (uint32_t)tv.tv_usec);
  if (!event_obj) {
    return -EINVAL;
  }
  evbase_t *base = evbase;
  if (!base) {
    return -EINVAL;
  }
  del_evtimer();  // clean up first, in case it was already added
  evt = event_obj->new_event(
      base, -1, EV_PERSIST, libevent_callback,
      static_cast<void *>(static_cast<RecurringEventBase *>(this)));
  if (!evt) {
    return -ENOMEM;
  }
  event_obj->add_event(evt, &tv);
  return 0;
}

void RecurringEventBase::del_evtimer() {
  s3_log(S3_LOG_DEBUG, "", "Entering");
  if (evt && event_obj) {
    event_obj->del_event(evt);
    event_obj->free_event(evt);
    evt = nullptr;
  }
}

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

#ifndef __S3_LIBEVENT_SOCKET_WRAPPER_H__
#define __S3_LIBEVENT_SOCKET_WRAPPER_H__

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>

class S3LibeventSocketWrapper {
 public:
  virtual ~S3LibeventSocketWrapper() {}

  virtual struct bufferevent *bufferevent_socket_new(struct event_base *base,
                                                     evutil_socket_t fd,
                                                     int options) {
    return ::bufferevent_socket_new(base, fd, options);
  }

  virtual void bufferevent_setcb(struct bufferevent *bufev,
                                 bufferevent_data_cb readcb,
                                 bufferevent_data_cb writecb,
                                 bufferevent_event_cb eventcb, void *cbarg) {
    ::bufferevent_setcb(bufev, readcb, writecb, eventcb, cbarg);
  }

  virtual int bufferevent_disable(struct bufferevent *bufev, short event) {
    return ::bufferevent_disable(bufev, event);
  }

  virtual int bufferevent_enable(struct bufferevent *bufev, short event) {
    return ::bufferevent_enable(bufev, event);
  }

  virtual int evbuffer_add(struct evbuffer *buf, const void *data,
                           size_t datlen) {
    return ::evbuffer_add(buf, data, datlen);
  }

  virtual int bufferevent_socket_connect_hostname(struct bufferevent *bufev,
                                                  struct evdns_base *evdns_base,
                                                  int family,
                                                  const char *hostname,
                                                  int port) {
    return ::bufferevent_socket_connect_hostname(bufev, evdns_base, family,
                                                 hostname, port);
  }

  virtual void bufferevent_free(struct bufferevent *bufev) {
    ::bufferevent_free(bufev);
  }

  virtual struct evdns_base *evdns_base_new(struct event_base *event_base,
                                            int initialize_nameservers) {
    return ::evdns_base_new(event_base, initialize_nameservers);
  }

  virtual void evdns_base_free(struct evdns_base *base, int fail_requests) {
    ::evdns_base_free(base, fail_requests);
  }
};

#endif

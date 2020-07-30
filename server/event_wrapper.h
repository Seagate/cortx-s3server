 */

#pragma once

#ifndef __S3_SERVER_EVENT_WRAPPER_H__
#define __S3_SERVER_EVENT_WRAPPER_H__

#include "s3_common.h"

EXTERN_C_BLOCK_BEGIN

#include <event2/event.h>

EXTERN_C_BLOCK_END

// A wrapper class for libevent functions so that we are able to mock
// c functions in tests. For Prod (non-test) this will just forward the calls.

//++
// Note: Its being observed that when wrapper name is same as that
// of actual event api in libevent "C" library, then s3server goes
// to hung state.
//--

class EventInterface {
 public:
  virtual ~EventInterface() {}
  virtual struct event *new_event(struct event_base *base, evutil_socket_t fd,
                                  short events,
                                  void (*cb)(evutil_socket_t, short, void *),
                                  void *arg) = 0;
  virtual void del_event(struct event *s3_client_event) = 0;
  virtual void free_event(struct event *s3_client_event) = 0;
  virtual bool pending_event(struct event *s3_client_event, short event,
                             struct timeval *tv = nullptr) = 0;
  virtual void add_event(struct event *s3_client_event, struct timeval *tv) = 0;
};

class EventWrapper : public EventInterface {
 public:
  struct event *new_event(struct event_base *base, evutil_socket_t fd,
                          short events,
                          void (*cb)(evutil_socket_t, short, void *),
                          void *arg);
  void del_event(struct event *s3_client_event);
  void free_event(struct event *s3_client_event);
  bool pending_event(struct event *s3_client_event, short event,
                     struct timeval *tv = nullptr);
  void add_event(struct event *s3_client_event, struct timeval *tv);
};
#endif

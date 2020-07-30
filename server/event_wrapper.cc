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

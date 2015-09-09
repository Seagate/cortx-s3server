

#include "s3_post_to_main_loop.h"

void S3PostToMainLoop::operator()(user_event_on_main_loop callback) {
  struct event *ev_user = NULL;
  struct event_base* base = request->get_evbase();

  ev_user = event_new(base, -1, EV_WRITE|EV_READ, callback, context);
  event_active(ev_user, EV_READ|EV_WRITE, 1);
}

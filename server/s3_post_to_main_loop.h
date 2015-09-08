

extern "C" typedef void (*user_event_on_main_loop)(evutil_socket_t, short events, void *user_data);

class S3PostToMainLoop {
  void* context;
  S3RequestObject* request;
public:
  S3PostToMainLoop(S3RequestObject* req, void* ctx) : request(req), context(ctx) {}

  void operator()(user_event_on_main_loop* callback) {
    struct event *ev_user = NULL;
    struct event_base* base = request->get_evbase();

    ev_user = event_new(base, -1, EV_WRITE|EV_READ, callback, context);
    event_active(ev_user, EV_READ|EV_WRITE, 1);
  }
}

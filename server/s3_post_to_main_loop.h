#pragma once

#ifndef __S3_SERVER_S3_POST_TO_MAIN_LOOP_H__
#define __S3_SERVER_S3_POST_TO_MAIN_LOOP_H__

/* libevhtp */
#include <evhtp.h>

// #include "s3_asyncop_context_base.h"
#include "s3_log.h"

struct user_event_context {
  void *app_ctx;
  void *user_event;
};

extern "C" typedef void (*user_event_on_main_loop)(evutil_socket_t,
                                                   short events,
                                                   void *user_data);

class S3PostToMainLoop {
  void *context;

 public:
  S3PostToMainLoop(void *ctx) : context(ctx) {
    s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  }

  void operator()(user_event_on_main_loop callback,
                  const std::string &request_id = "");
};

#endif

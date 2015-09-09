
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_POST_TO_MAIN_LOOP_H__
#define __MERO_FE_S3_SERVER_S3_POST_TO_MAIN_LOOP_H__

/* libevhtp */
#include <evhtp.h>

#include "s3_request_object.h"

extern "C" typedef void (*user_event_on_main_loop)(evutil_socket_t, short events, void *user_data);

class S3PostToMainLoop {
  void* context;
  std::shared_ptr<S3RequestObject> request;
public:
  S3PostToMainLoop(std::shared_ptr<S3RequestObject> req, void* ctx) : request(req), context(ctx) {}

  void operator()(user_event_on_main_loop* callback);
};

#endif

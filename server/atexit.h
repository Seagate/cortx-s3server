#pragma once

#ifndef __S3_SERVER_ATEXIT_H__

#include <functional>

class AtExit {
 private:
  std::function<void()> handler;
  bool enabled = true;

 public:
  AtExit(std::function<void()> handler_) : handler(handler_) {}
  ~AtExit() { call_now(); }
  void cancel() { enabled = false; }
  void reenable() { enabled = true; }
  void call_now() {
    if (enabled && handler) handler();
    enabled = false;
  }
};

#endif

#pragma once

#ifndef __S3_SERVER_S3_DAEMONIZE_SERVER_H__
#define __S3_SERVER_S3_DAEMONIZE_SERVER_H__

#include <signal.h>
#include <string>
#include "s3_option.h"

class S3Daemonize {
  int noclose;
  std::string pidfilename;
  int write_to_pidfile();
  S3Option *option_instance;

 public:
  S3Daemonize();
  void daemonize();
  void wait_for_termination();
  int delete_pidfile();
  void register_signals();

  void set_fatal_handler_exit();
  void set_fatal_handler_graceful();

  int get_s3daemon_redirection();
};

#endif

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

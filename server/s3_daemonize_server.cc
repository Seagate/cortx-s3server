/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 12-April-2016
 */

#include "s3_daemonize_server.h"
#include "clovis_helpers.h"
#include "s3_log.h"
#include "s3_option.h"
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <execinfo.h>
#include <evhtp.h>

#define S3_BACKTRACE_DEPTH_MAX 256
#define ARRAY_SIZE(a) ((sizeof(a))/(sizeof(a)[0]))

extern evbase_t * global_evbase_handle;

void s3_terminate_sig_handler(int signum) {
  s3_log(S3_LOG_INFO, "Recieved signal %d, shutting down s3 server daemon...\n",signum);
  //++
  //Exit event base loop immediately
  //If the event_base is currently running callbacks for any active events, it will exit immediately
  //after finishing the one it’s currently processing.
  //--
  event_base_loopbreak(global_evbase_handle);
  // Handle inconsistency via rollbacks ... -- TODO
  return;
}

void s3_terminate_fatal_handler(int signum) {
  void *trace[S3_BACKTRACE_DEPTH_MAX];
  std::string bt_str = "";
  std::string newline_str("\n");
  char **buf;
  int rc;
  rc = backtrace(trace, ARRAY_SIZE(trace));
  buf = backtrace_symbols(trace, rc);
  for(int i = 0; i < rc; i++) {
    bt_str += buf[i] + newline_str;
  }
  s3_log(S3_LOG_FATAL, "Backtrace:\n%s\n",bt_str.c_str());
  free(buf);
  S3Daemonize s3daemon;
  s3daemon.delete_pidfile();
  //dafault handler for core dumping
  raise(signum);
}

std::string S3Daemonize::pidfilename = "";

S3Daemonize::S3Daemonize(): noclose(0) {
    if (S3Option::get_instance()->do_redirection() == 0) {
      noclose = 1;
    }
}

void S3Daemonize::daemonize() {
  int rc;
  std::string daemon_wd;
  struct sigaction s3hup_act;
  s3hup_act.sa_handler = SIG_IGN;
  rc = daemon(1, noclose);
  if (rc) {
    s3_log(S3_LOG_ERROR, "Failed to daemonize s3 server, errno = %d\n",errno);
    exit(1);
  }
  sigaction(SIGHUP, &s3hup_act, NULL);

  daemon_wd = S3Option::get_instance()->get_daemon_dir();
  if(access(daemon_wd.c_str(), F_OK) != 0) {
    s3_log(S3_LOG_FATAL, "The directory %s doesn't exist, errno = %d\n",daemon_wd.c_str(), errno);
    exit(1);
  }

  if(::chdir(daemon_wd.c_str())) {
    s3_log(S3_LOG_FATAL, "Failed to chdir to %s, errno = %d\n",daemon_wd.c_str(), errno);
    exit(1);
  }
  write_to_pidfile();
}

int S3Daemonize::write_to_pidfile() {
  std::string pidstr;
  std::ofstream pidfile;
  pidstr = std::to_string(getpid());
  S3Daemonize::pidfilename = "/var/run/s3server-" + pidstr + ".pid";
  pidfile.open(pidfilename);
  if (pidfile.fail()) {
    s3_log(S3_LOG_ERROR, "Failed to open pid file %s\n errno = %d", S3Daemonize::pidfilename.c_str(), errno);
    goto FAIL;
  }
  if (!(pidfile << pidstr)) {
     s3_log(S3_LOG_ERROR, "Failed to write to pid file %s errno = %d\n", S3Daemonize::pidfilename.c_str(), errno);
     goto FAIL;
  }
  pidfile.close();
  return 0;
FAIL:
  return -1;
}

int S3Daemonize::delete_pidfile() {
  char pidstr_read[100];
  int rc;
  std::ifstream pidfile_read;
  s3_log(S3_LOG_DEBUG, "Entering");
  if(S3Daemonize::pidfilename == "") {
    s3_log(S3_LOG_ERROR, "pid filename %s doesn't exist\n", S3Daemonize::pidfilename.c_str());
    return 0;
  }
  pidfile_read.open(S3Daemonize::pidfilename);
  if (pidfile_read.fail()) {
    s3_log(S3_LOG_ERROR, "Failed to open pid file %s errno = %d\n", S3Daemonize::pidfilename.c_str(), errno);
    return -1;
  }
  pidfile_read.getline (pidstr_read,100);
  if (strlen(pidstr_read) == 0) {
    s3_log(S3_LOG_ERROR, "Pid doesn't exist within %s\n", S3Daemonize::pidfilename.c_str());
    return -1;
  }
  pidfile_read.close();
  if (pidstr_read != std::to_string(getpid())) {
    s3_log(S3_LOG_WARN, "The pid(%d) of process does match to the pid(%s) in the pid file %s\n",
           getpid(), pidstr_read, S3Daemonize::pidfilename.c_str());
    return -1;
  }
  rc = ::unlink(S3Daemonize::pidfilename.c_str());
  if (rc) {
    return rc;
  }
  s3_log(S3_LOG_DEBUG, "Exiting");
  return 0;
}

void S3Daemonize::register_signals() {
  struct sigaction s3action;
  struct sigaction fatal_action;
  s3action.sa_handler = s3_terminate_sig_handler;
  sigemptyset(&s3action.sa_mask);
  s3action.sa_flags = 0;
  sigaction(SIGTERM, &s3action, NULL);
  sigaction(SIGINT, &s3action, NULL);

  fatal_action.sa_handler = s3_terminate_fatal_handler;
  sigemptyset(&fatal_action.sa_mask);
  // Call default signal handler if at all heap corruption creates another SIGSEGV within signal handler
  fatal_action.sa_flags = SA_RESETHAND | SA_NODEFER;
  sigaction(SIGSEGV, &fatal_action, NULL);
  sigaction(SIGABRT, &fatal_action, NULL);
  sigaction(SIGILL, &fatal_action, NULL);
  sigaction(SIGFPE, &fatal_action, NULL);
  sigaction(SIGSYS, &fatal_action, NULL);
  sigaction(SIGXFSZ, &fatal_action, NULL);
  sigaction(SIGXCPU, &fatal_action, NULL);
}

int S3Daemonize::get_s3daemon_redirection() {
  return noclose;
}

std::string S3Daemonize::get_s3_pid_filename() {
  return S3Daemonize::pidfilename;
}

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

#include "s3_daemonize_server.h"
#include <evhtp.h>
#include <execinfo.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "motr_helpers.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_option.h"

#define S3_BACKTRACE_DEPTH_MAX 2048
#define S3_ARRAY_SIZE(a) ((sizeof(a)) / (sizeof(a)[0]))

extern evbase_t *global_evbase_handle;
extern int global_shutdown_in_progress;


/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047005001</event_code>
 *    <application>S3 Server</application>
 *    <submodule>General</submodule>
 *    <description>Fatal handler triggered</description>
 *    <audience>Service</audience>
 *    <details>
 *      Fatal handler has triggered. This will cause S3 Server to crash.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *        signal_number - signal which invoked the handler.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files. Restart S3 server and contact development
 *      team for further investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

void s3_terminate_fatal_handler(int signum) {

  if (S3Option::get_instance()->do_redirection() == 0) {
    void *trace[S3_BACKTRACE_DEPTH_MAX];
    int rc = backtrace(trace, S3_ARRAY_SIZE(trace));
    backtrace_symbols_fd(trace, rc, STDERR_FILENO);
  }
  S3Option *option_instance = S3Option::get_instance();
  std::string pid_file = option_instance->get_s3_pidfile();
  if (!pid_file.empty()) {
    ::unlink(pid_file.c_str());
  }
  raise(signum);
}

S3Daemonize::S3Daemonize() : noclose(0) {
  option_instance = S3Option::get_instance();
  if (option_instance->do_redirection() == 0) {
    noclose = 1;
  }
  pidfilename = option_instance->get_s3_pidfile();
}

void S3Daemonize::daemonize() {
  int rc;
  std::string daemon_wd;

  struct sigaction s3hup_act;
  memset(&s3hup_act, 0, sizeof s3hup_act);
  s3hup_act.sa_handler = SIG_IGN;

  rc = daemon(1, noclose);
  if (rc) {
    s3_log(S3_LOG_FATAL, "", "Failed to daemonize s3 server, errno = %d\n",
           errno);
    exit(1);
  }
  sigaction(SIGHUP, &s3hup_act, NULL);

  // Set the working directory for current instance as s3server-process_fid
  std::string process_fid = option_instance->get_motr_process_fid();
  // Remove the surrounding angle brackets <>
  process_fid.erase(0, 1);
  process_fid.erase(process_fid.size() - 1);

  daemon_wd = option_instance->get_daemon_dir() + "/s3server-" + process_fid;
  if (access(daemon_wd.c_str(), F_OK) != 0) {
    s3_log(S3_LOG_FATAL, "", "The directory %s doesn't exist, errno = %d\n",
           daemon_wd.c_str(), errno);
  }

  if (::chdir(daemon_wd.c_str())) {
    s3_log(S3_LOG_FATAL, "", "Failed to chdir to %s, errno = %d\n",
           daemon_wd.c_str(), errno);
  }
  s3_log(S3_LOG_INFO, "", "Working directory for S3 server = [%s]\n",
         daemon_wd.c_str());
  write_to_pidfile();
}

int S3Daemonize::write_to_pidfile() {
  std::string pidstr;
  std::ofstream pidfile;
  pidstr = std::to_string(getpid());
  pidfile.open(pidfilename);
  if (pidfile.fail()) {
    s3_log(S3_LOG_ERROR, "", "Failed to open pid file %s\n errno = %d",
           pidfilename.c_str(), errno);
    goto FAIL;
  }
  if (!(pidfile << pidstr)) {
    s3_log(S3_LOG_ERROR, "", "Failed to write to pid file %s errno = %d\n",
           pidfilename.c_str(), errno);
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
  s3_log(S3_LOG_DEBUG, "", "Entering");
  if (pidfilename == "") {
    s3_log(S3_LOG_ERROR, "", "pid filename %s doesn't exist\n",
           pidfilename.c_str());
    s3_log(S3_LOG_DEBUG, "", "Exiting");
    return 0;
  }
  pidfile_read.open(S3Daemonize::pidfilename);
  if (pidfile_read.fail()) {
    if (errno == 2) {
      s3_log(S3_LOG_DEBUG, "", "Exiting");
      return 0;
    } else {
      s3_log(S3_LOG_ERROR, "", "Failed to open pid file %s errno = %d\n",
             pidfilename.c_str(), errno);
    }
    s3_log(S3_LOG_DEBUG, "", "Exiting");
    return -1;
  }
  pidfile_read.getline(pidstr_read, 100);
  if (strlen(pidstr_read) == 0) {
    s3_log(S3_LOG_ERROR, "", "Pid doesn't exist within %s\n",
           pidfilename.c_str());
    s3_log(S3_LOG_DEBUG, "", "Exiting");
    return -1;
  }
  pidfile_read.close();
  if (pidstr_read != std::to_string(getpid())) {
    s3_log(
        S3_LOG_WARN, "-",
        "The pid(%d) of process does match to the pid(%s) in the pid file %s\n",
        getpid(), pidstr_read, pidfilename.c_str());
    s3_log(S3_LOG_DEBUG, "", "Exiting");
    return -1;
  }
  rc = ::unlink(pidfilename.c_str());
  if (rc) {
    s3_log(S3_LOG_WARN, "", "File %s deletion failed\n", pidfilename.c_str());
    s3_log(S3_LOG_DEBUG, "", "Exiting");
    return rc;
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting");
  return 0;
}

void S3Daemonize::register_signals() {
  struct sigaction fatal_action;
  memset(&fatal_action, 0, sizeof fatal_action);

  fatal_action.sa_handler = s3_terminate_fatal_handler;
  // Call default signal handler if at all heap corruption creates another
  // SIGSEGV within signal handler
  fatal_action.sa_flags = SA_RESETHAND | SA_NODEFER;

  sigaction(SIGSEGV, &fatal_action, NULL);
  sigaction(SIGABRT, &fatal_action, NULL);
  sigaction(SIGILL, &fatal_action, NULL);
  sigaction(SIGFPE, &fatal_action, NULL);
}

int S3Daemonize::get_s3daemon_redirection() { return noclose; }

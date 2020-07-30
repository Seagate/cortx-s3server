#include <evhtp.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "gtest/gtest.h"
#include "s3_daemonize_server.h"
#include "s3_option.h"

evbase_t *global_evbase_handle;

class S3DaemonizeTest : public testing::Test {
 protected:
  S3DaemonizeTest() { option_instance = S3Option::get_instance(); }
  S3Option *option_instance;
};

TEST_F(S3DaemonizeTest, DefaultCons) {
  S3Daemonize s3daemon;
  EXPECT_EQ(0, s3daemon.get_s3daemon_redirection());
}

TEST_F(S3DaemonizeTest, SetOptionsCons) {
  option_instance->set_redirection(0);
  S3Daemonize s3daemon;
  EXPECT_EQ(1, s3daemon.get_s3daemon_redirection());
}

#if 0
TEST_F(S3DaemonizeTest, Daemonize) {
  char cwd[256];
  S3Daemonize s3daemon;
  pid_t pid_initial = getpid();
  getcwd(cwd, sizeof(cwd));
  s3daemon.daemonize();
  pid_t pid_after_daemon = getpid();
  ASSERT_TRUE(pid_after_daemon != pid_initial);
  std::string filename = "/var/run/s3server-" + std::to_string(pid_after_daemon) + ".pid";
  ASSERT_TRUE(access(filename.c_str(), F_OK ) != -1);
  ASSERT_TRUE(filename == s3daemon.get_s3_pid_filename());

  // Delete the pid file
  s3daemon.delete_pidfile();
  ASSERT_TRUE(access(filename.c_str(), F_OK ) == -1);

  chdir(cwd);
}
#endif

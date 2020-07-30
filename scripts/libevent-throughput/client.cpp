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


#include <cstdlib>
#include <cstring>
#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <gflags/gflags.h>

DEFINE_string(ip, "127.0.0.1", "Server's IP address");
DEFINE_int32(port, 60001, "TCP port for connection");
DEFINE_bool(recv, false, "Receive data from the client");

char buff[4096];

static bool f_exit;

static void sig_handler(int s)
{
  f_exit = true;
}

int main(int argc, char **argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  struct sockaddr_in sin;
  memset(&sin, 0, sizeof sin);

  if (!inet_aton(FLAGS_ip.c_str(), &sin.sin_addr))
  {
    std::cout << "Bad IP address\n";
    return 1;
  }
  if (FLAGS_port < 1024 || FLAGS_port > 65535)
  {
    std::cout << "Illegal port number (" << FLAGS_port << ")\n";
    return 1;
  }
  sin.sin_family = AF_INET;
  sin.sin_port = htons(FLAGS_port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0)
  {
    perror("socket()");
    return 1;
  }
  if (connect(fd, (struct sockaddr*)&sin, sizeof sin) < 0)
  {
    perror("connect()");
    return 1;
  }
  signal(SIGINT, sig_handler);

  unsigned long long total_bytes = 0;
  time_t start_time = time(nullptr);
  ssize_t bytes;

  while(!f_exit)
  {
    if (FLAGS_recv)
    {
      bytes = recv(fd, buff, sizeof buff, 0);
    }
    else
    {
      bytes = send(fd, buff, sizeof buff, 0);
    }
    if (bytes <=0 )
    {
      break;
    }
    total_bytes += bytes;
  }
  time_t elapsed = time(nullptr) - start_time;
  unsigned mbs = total_bytes / (1024 * 1024);

  std::cout << "Total " << mbs << " MBs during " << elapsed << "sec\n"
            << "Speed: " << mbs / elapsed << " MB/s\n";

  return 0;
}

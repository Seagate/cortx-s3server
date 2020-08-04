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

#ifndef __S3_SERVER_SOCKET_WRAPPER_H__
#define __S3_SERVER_SOCKET_WRAPPER_H__

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// A wrapper class for socket system calls so that we are able to mock those
// syscalls in tests. For Prod (non-test) this will just forward the calls.
class SocketInterface {
 public:
  virtual int socket(int domain, int type, int protocol) = 0;
  virtual int fcntl(int fd, int cmd, int flags) = 0;
  virtual int inet_aton(const char *cp, struct in_addr *inp) = 0;
  virtual int close(int fd) = 0;
  virtual ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                         const struct sockaddr *dest_addr,
                         socklen_t addrlen) = 0;
  virtual ~SocketInterface(){};
};

class SocketWrapper : public SocketInterface {
 public:
  int socket(int domain, int type, int protocol) {
    return ::socket(domain, type, protocol);
  }

  int fcntl(int fd, int cmd, int flags) { return ::fcntl(fd, cmd, flags); }

  int inet_aton(const char *cp, struct in_addr *inp) {
    return ::inet_aton(cp, inp);
  }

  int close(int fd) { return ::close(fd); }

  ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                 const struct sockaddr *dest_addr, socklen_t addrlen) {
    return ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
  }
};

#endif  // __S3_SERVER_SOCKET_WRAPPER_H__

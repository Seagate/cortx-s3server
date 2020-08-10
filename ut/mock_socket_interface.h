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

#ifndef __UT_MOCK_SOCKET_INTERFACE_H__
#define __UT_MOCK_SOCKET_INTERFACE_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_stats.h"

/* Mock class compliant with Google Mock framework.
 * This class mocks SocketInterface defined in server/socket_wrapper.h.
*/
class MockSocketInterface : public SocketInterface {
 public:
  MOCK_METHOD3(socket, int(int domain, int type, int protocol));
  MOCK_METHOD3(fcntl, int(int fd, int cmd, int flags));
  MOCK_METHOD2(inet_aton, int(const char *cp, struct in_addr *inp));
  MOCK_METHOD1(close, int(int fd));
  MOCK_METHOD6(sendto,
               ssize_t(int sockfd, const void *buf, size_t len, int flags,
                       const struct sockaddr *dest_addr, socklen_t addrlen));
};

#endif

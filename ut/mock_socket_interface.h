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

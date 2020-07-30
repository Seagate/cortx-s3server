#pragma once

#ifndef __S3_UT_S3_TEST_UTILS_H__
#define __S3_UT_S3_TEST_UTILS_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define EXPECT_OID_EQ(O1, O2)  \
  EXPECT_EQ(O1.u_lo, O2.u_lo); \
  EXPECT_EQ(O1.u_hi, O2.u_hi);

#define EXPECT_OID_NE(O1, O2)  \
  EXPECT_NE(O1.u_lo, O2.u_lo); \
  EXPECT_NE(O1.u_hi, O2.u_hi);

#endif

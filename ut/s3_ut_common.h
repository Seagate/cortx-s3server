#pragma once

#ifndef __S3_UT_COMMON_H__
#define __S3_UT_COMMON_H__

#include "clovis_helpers.h"

// Common functions for ut

static int dummy_helpers_ufid_next(struct m0_uint128 *ufid) {
  unsigned int rand1 = rand();
  unsigned int rand2 = rand();
  *ufid = {rand1, rand2};
  return 0;
}

#endif

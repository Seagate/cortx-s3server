 */

#pragma once

#ifndef __S3_UT_S3_CALLBACK_TEST_HELPERS_H__
#define __S3_UT_S3_CALLBACK_TEST_HELPERS_H__

#include "clovis_helpers.h"
#include "s3_clovis_rw_common.h"

class S3CallBack {
 public:
  S3CallBack() { success_called = fail_called = false; }

  void on_success() { success_called = true; }

  void on_failed() { fail_called = true; }
  bool success_called;
  bool fail_called;
};

#endif

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

#ifndef __S3_UT_S3_CALLBACK_TEST_HELPERS_H__
#define __S3_UT_S3_CALLBACK_TEST_HELPERS_H__

#include "motr_helpers.h"
#include "s3_motr_rw_common.h"

class S3CallBack {
 public:
  S3CallBack() { success_called = fail_called = false; }

  void on_success() { success_called = true; }

  void on_failed() { fail_called = true; }
  bool success_called;
  bool fail_called;
};

#endif

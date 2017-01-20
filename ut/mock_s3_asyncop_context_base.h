/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar <rajesh.nambiar@seagate.com>
 * Original creation date: 19-Nov-2015
 */

#pragma once

#ifndef __S3_UT_MOCK_S3_ASYNCOP_CONTEXT_BASE_H__
#define __S3_UT_MOCK_S3_ASYNCOP_CONTEXT_BASE_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_asyncop_context_base.h"
#include "s3_common.h"

class MockS3AsyncOpContextBase : public S3AsyncOpContextBase {
 public:
  MockS3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req,
                           std::function<void(void)> success_callback,
                           std::function<void(void)> failed_callback)
      : S3AsyncOpContextBase(req, success_callback, failed_callback) {}
  MOCK_METHOD2(set_op_status,
               void(S3AsyncOpStatus opstatus, std::string message));
};

#endif

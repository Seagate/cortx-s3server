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
 * Original creation date: 22-Nov-2015
 */

#pragma once

#ifndef __S3_UT_MOCK_S3_CLOVIS_KVS_WRITER_H__
#define __S3_UT_MOCK_S3_CLOVIS_KVS_WRITER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockS3ClovisKVSWriter : public S3ClovisKVSWriter {
 public:
  MockS3ClovisKVSWriter(std::shared_ptr<S3RequestObject> req,
                        std::shared_ptr<ClovisAPI> s3_clovis_api)
      : S3ClovisKVSWriter(req, s3_clovis_api) {}
};
#endif

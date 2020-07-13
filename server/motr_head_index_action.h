/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Amit Kumar   <amit.kumar@seagate.com>
 * Original creation date: 06-Apr-2020
 */

#ifndef __MOTR_HEAD_INDEX_ACTION_H__
#define __MOTR_HEAD_INDEX_ACTION_H__

#include <memory>
#include <gtest/gtest_prod.h>

#include "s3_factory.h"
#include "motr_action_base.h"

class MotrHeadIndexAction : public MotrAction {
  m0_uint128 index_id;
  std::shared_ptr<ClovisAPI> motr_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;

  void setup_steps();
  void validate_request();
  void check_index_exist();
  void check_index_exist_success();
  void check_index_exist_failure();
  void send_response_to_s3_client();

 public:
  MotrHeadIndexAction(
      std::shared_ptr<MotrRequestObject> req,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_motr_kvs_reader_factory =
          nullptr);

  FRIEND_TEST(MotrHeadIndexActionTest, ValidIndexId);
  FRIEND_TEST(MotrHeadIndexActionTest, InvalidIndexId);
  FRIEND_TEST(MotrHeadIndexActionTest, EmptyIndexId);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExist);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistSuccess);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistFailureMissing);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistFailureInternalError);
  FRIEND_TEST(MotrHeadIndexActionTest, CheckIndexExistFailureFailedToLaunch);
  FRIEND_TEST(MotrHeadIndexActionTest, SendSuccessResponse);
  FRIEND_TEST(MotrHeadIndexActionTest, SendBadRequestResponse);
};
#endif

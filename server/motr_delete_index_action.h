/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 30-March-2020
 */

#pragma once

#ifndef __S3_SERVER_MOTR_INDEX_DELETE_ACTION_H__
#define __S3_SERVER_MOTR_INDEX_DELETE_ACTION_H__

#include <memory>
#include <gtest/gtest_prod.h>

#include "motr_action_base.h"
#include "s3_clovis_kvs_writer.h"

class MotrDeleteIndexAction : public MotrAction {
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;
  std::shared_ptr<ClovisAPI> motr_clovis_api;
  std::shared_ptr<S3ClovisKVSWriterFactory> motr_clovis_kvs_writer_factory;

  m0_uint128 index_id;

 public:
  MotrDeleteIndexAction(
      std::shared_ptr<MotrRequestObject> req,
      std::shared_ptr<S3ClovisKVSWriterFactory> clovis_kvs_reader_factory =
          nullptr);
  void setup_steps();

  void validate_request();
  void delete_index();
  void delete_index_successful();
  void delete_index_failed();

  void send_response_to_s3_client();

  FRIEND_TEST(MotrDeleteIndexActionTest, ConstructorTest);
  FRIEND_TEST(MotrDeleteIndexActionTest, ValidIndexId);
  FRIEND_TEST(MotrDeleteIndexActionTest, InvalidIndexId);
  FRIEND_TEST(MotrDeleteIndexActionTest, EmptyIndexId);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndex);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexSuccess);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexFailedIndexMissing);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexFailed);
  FRIEND_TEST(MotrDeleteIndexActionTest, DeleteIndexFailedToLaunch);
  FRIEND_TEST(MotrDeleteIndexActionTest, SendSuccessResponse);
  FRIEND_TEST(MotrDeleteIndexActionTest, SendBadRequestResponse);
};

#endif

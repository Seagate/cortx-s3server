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
 * Original author: Siddhivinayak Shanbhag  <siddhivinayak.shanbhag@seagate.com>
 * Original author: Amit Kumar              <amit.kumar@seagate.com>
 * Original creation date: 16-Jun-2020
 */

#pragma once

#ifndef __MERO_PUT_KEY_VALUE_ACTION_H__
#define __MERO_PUT_KEY_VALUE_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_factory.h"
#include "mero_action_base.h"

class MeroPutKeyValueAction : public MeroAction {
  m0_uint128 index_id;
  std::string json_value;
  std::shared_ptr<ClovisAPI> mero_clovis_api;
  std::shared_ptr<S3ClovisKVSWriter> clovis_kv_writer;
  std::shared_ptr<S3ClovisKVSWriterFactory> clovis_kvs_writer_factory;

  bool is_valid_json(std::string);

 public:
  MeroPutKeyValueAction(
      std::shared_ptr<MeroRequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr,
      std::shared_ptr<S3ClovisKVSWriterFactory> clovis_mero_kvs_writer_factory =
          nullptr);

  void setup_steps();
  void read_and_validate_key_value();
  void put_key_value();
  void put_key_value_successful();
  void put_key_value_failed();
  void consume_incoming_content();
  void send_response_to_s3_client();

  FRIEND_TEST(MeroPutKeyValueActionTest, ValidateKeyValueValidIndexValidValue);
  FRIEND_TEST(MeroPutKeyValueActionTest, PutKeyValueSuccessful);
  FRIEND_TEST(MeroPutKeyValueActionTest, PutKeyValue);
  FRIEND_TEST(MeroPutKeyValueActionTest, PutKeyValueFailed);
  FRIEND_TEST(MeroPutKeyValueActionTest, ValidJson);
  FRIEND_TEST(MeroPutKeyValueActionTest, InvalidJson);
};
#endif
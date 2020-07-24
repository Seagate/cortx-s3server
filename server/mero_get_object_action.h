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
 * Original author:  Siddhivinayak Shanbhag
 *<sidddhivinayak.shanbhag@seagate.com>
 * Original creation date: 24-July-2020
 */

#pragma once

#ifndef __MERO_GET_OBJECT_ACTION_H__
#define __MERO_GET_OBJECT_ACTION_H__

#include <gtest/gtest_prod.h>
#include <memory>

#include "s3_factory.h"
#include "mero_action_base.h"

class MeroGetObjectAction : public MeroAction {

  std::shared_ptr<S3ClovisReader> clovis_reader;
  // Read state
  size_t total_blocks_in_object;
  size_t blocks_already_read;
  size_t data_sent_to_client;
  size_t content_length;

  size_t first_byte_offset_to_read;
  size_t last_byte_offset_to_read;
  size_t total_blocks_to_read;

  bool read_object_reply_started;
  std::shared_ptr<S3ClovisReaderFactory> clovis_reader_factory;

  int layout_id;
  m0_uint128 oid;
  std::shared_ptr<S3ClovisWriter> clovis_writer;

 public:
  MeroGetObjectAction(std::shared_ptr<MeroRequestObject> req,
                      std::shared_ptr<S3ClovisReaderFactory> reader_factory =
                          nullptr);

  void setup_steps();
  void validate_request();

  void read_object();
  void read_object_data();
  void read_object_data_failed();
  void set_total_blocks_to_read_from_object();
  void send_data_to_client();
  size_t get_requested_content_length() const {
    return last_byte_offset_to_read - first_byte_offset_to_read + 1;
  }

  void send_response_to_s3_client();
};
#endif
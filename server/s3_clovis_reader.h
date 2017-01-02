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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_READER_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_READER_H__

#include <memory>
#include <functional>

#include "s3_request_object.h"
#include "s3_clovis_context.h"
#include "s3_asyncop_context_base.h"
#include "s3_clovis_wrapper.h"
#include "s3_log.h"

extern S3Option* g_option_instance;

class S3ClovisReaderContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_op_context* clovis_op_context;
  bool has_clovis_op_context;

  // Read/Write Operation context.
  struct s3_clovis_rw_op_context* clovis_rw_op_context;
  bool has_clovis_rw_op_context;

public:
  S3ClovisReaderContext(std::shared_ptr<S3RequestObject> req,
      std::function<void()> success_callback, std::function<void()> failed_callback) :
      S3AsyncOpContextBase(req, success_callback, failed_callback) {
    s3_log(S3_LOG_DEBUG, "Constructor\n");

    // Create or write, we need op context
    clovis_op_context = create_basic_op_ctx(1);
    has_clovis_op_context = true;

    clovis_rw_op_context = NULL;
    has_clovis_rw_op_context = false;
  }

  ~S3ClovisReaderContext() {
    s3_log(S3_LOG_DEBUG, "Destructor\n");

    if (has_clovis_op_context) {
      free_basic_op_ctx(clovis_op_context);
    }
    if (has_clovis_rw_op_context) {
      free_basic_rw_op_ctx(clovis_rw_op_context);
    }
  }

  // Call this when you want to do write op.
  uint64_t init_read_op_ctx(size_t clovis_block_count, uint64_t last_index) {
    clovis_rw_op_context = create_basic_rw_op_ctx(clovis_block_count);
    has_clovis_rw_op_context = true;

    for (size_t i = 0; i < clovis_block_count; i++) {
      clovis_rw_op_context->ext->iv_index[i] = last_index ;
      clovis_rw_op_context->ext->iv_vec.v_count[i] =
          g_option_instance->get_clovis_block_size();
      last_index += g_option_instance->get_clovis_block_size();

      /* we don't want any attributes */
      clovis_rw_op_context->attr->ov_vec.v_count[i] = 0;
    }
    return last_index;  // where next read should start
  }

  struct s3_clovis_op_context* get_clovis_op_ctx() {
    return clovis_op_context;
  }

  struct s3_clovis_rw_op_context* get_clovis_rw_op_ctx() {
    return clovis_rw_op_context;
  }

  struct s3_clovis_rw_op_context* get_ownership_clovis_rw_op_ctx() {
    has_clovis_rw_op_context = false;  // release ownership, caller should free.
    return clovis_rw_op_context;
  }

};

enum class S3ClovisReaderOpState {
  failed,
  start,
  success,
  missing,  // Missing object
};

class S3ClovisReader {
private:
  std::shared_ptr<S3RequestObject> request;
  std::unique_ptr<S3ClovisReaderContext> reader_context;
  std::shared_ptr<ClovisAPI> s3_clovis_api;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  struct m0_uint128 oid;

  S3ClovisReaderOpState state;

  // Holds references to buffers after the read so it can be consumed.
  struct s3_clovis_rw_op_context* clovis_rw_op_context;
  size_t iteration_index;
  // to Help iteration.
  size_t num_of_blocks_read;

  uint64_t last_index;

public:
  //struct m0_uint128 id;
  //object id for object is generated within this constructor
  S3ClovisReader(std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api);
  //object id is generated at upper level and passed to this constructor
  S3ClovisReader(std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api, struct m0_uint128 id);
  S3ClovisReaderOpState get_state() {
    return state;
  }

  void set_oid(struct m0_uint128 id) {
    //TODO
    // oid = id;
  }

  // async read
  void read_object_data(size_t num_of_blocks, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void read_object_data_successful();
  void read_object_data_failed();

  // Iterate over the content.
  // Returns size of data in first block and 0 if there is no content,
  // and content in data.
  size_t get_first_block(char** data);
  size_t get_next_block(char** data);

  // For Testing purpose
  FRIEND_TEST(S3ClovisReaderTest, Constructor);
  FRIEND_TEST(S3ClovisReaderTest, ReadObjectDataSuccessStatusAndSuccessCallback);
};

#endif

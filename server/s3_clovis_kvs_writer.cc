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

#include <unistd.h>

#include "s3_common.h"

#include "s3_clovis_rw_common.h"
#include "s3_option.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_realm     clovis_uber_realm;
extern struct m0_clovis_container clovis_container;

S3ClovisKVSWriter::S3ClovisKVSWriter(std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> s3clovis_api) : request(req), s3_clovis_api(s3clovis_api), state(S3ClovisKVSWriterOpState::start) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  ops_count = 0;
}

S3ClovisKVSWriter::~S3ClovisKVSWriter() {
  s3_log(S3_LOG_DEBUG, "Destructor\n");
}

void S3ClovisKVSWriter::create_index(std::string index_name, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "index_name = %s\n", index_name.c_str());
  int rc = 0;

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::create_index_successful, this), std::bind( &S3ClovisKVSWriter::create_index_failed, this)));

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_ctx->cbs->oop_executed = NULL;
  idx_ctx->cbs->oop_stable = s3_clovis_op_stable;
  idx_ctx->cbs->oop_failed = s3_clovis_op_failed;


  S3UriToMeroOID(index_name.c_str(), &id);
  s3_clovis_api->clovis_idx_init(idx_ctx->idx, &clovis_uber_realm, &id);
  rc = s3_clovis_api->clovis_entity_create(&idx_ctx->idx->in_entity, &(idx_ctx->ops[0]));
  if(rc != 0) {
    s3_log(S3_LOG_ERROR, "m0_clovis_entity_create failed\n");
  }

  idx_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);

  writer_context->start_timer_for("create_index_op");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, 1, ClovisOpType::createidx);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::create_index_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSWriterOpState::saved;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::create_index_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (writer_context->get_errno_for(0) == -EEXIST) {
    state = S3ClovisKVSWriterOpState::exists;
    s3_log(S3_LOG_DEBUG, "Index already exists\n");
  } else {
    s3_log(S3_LOG_DEBUG, "Index creation failed\n");
    state = S3ClovisKVSWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_index(std::string index_name, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "index_name = %s\n", index_name.c_str());
  int rc = 0;

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::delete_index_successful, this), std::bind( &S3ClovisKVSWriter::delete_index_failed, this)));

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_ctx->cbs->oop_executed = NULL;
  idx_ctx->cbs->oop_stable = s3_clovis_op_stable;
  idx_ctx->cbs->oop_failed = s3_clovis_op_failed;


  S3UriToMeroOID(index_name.c_str(), &id);
  s3_clovis_api->clovis_idx_init(idx_ctx->idx, &clovis_uber_realm, &id);
  rc = s3_clovis_api->clovis_entity_delete(&idx_ctx->idx->in_entity, &(idx_ctx->ops[0]));
  if(rc != 0) {
    s3_log(S3_LOG_DEBUG, "m0_clovis_entity_delete failed\n");
  }

  idx_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);

  writer_context->start_timer_for("delete_index_op");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_index_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_index_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (writer_context->get_errno_for(0) == -ENOENT) {
    s3_log(S3_LOG_DEBUG, "Index doesn't exists\n");
    state = S3ClovisKVSWriterOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Deletion of Index failed\n");
    state = S3ClovisKVSWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_indexes(std::vector<struct m0_uint128> oids, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::delete_indexes_successful, this), std::bind( &S3ClovisKVSWriter::delete_indexes_failed, this), oids.size()));

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();


  ops_count = oids.size();
  struct m0_uint128 id;
  for (int i = 0; i < ops_count; ++i) {
    id = oids[i];
    struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context = (void *)writer_context.get();

    idx_ctx->cbs[i].oop_executed = NULL;
    idx_ctx->cbs[i].oop_stable = s3_clovis_op_stable;
    idx_ctx->cbs[i].oop_failed = s3_clovis_op_failed;

    s3_clovis_api->clovis_idx_init(&idx_ctx->idx[i], &clovis_uber_realm, &id);

    s3_clovis_api->clovis_entity_delete(&(idx_ctx->idx[i].in_entity), &(idx_ctx->ops[i]));

    idx_ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_clovis_api->clovis_op_setup(idx_ctx->ops[i], &idx_ctx->cbs[i], 0);
  }


  writer_context->start_timer_for("delete_index_op");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, oids.size());
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_indexes_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_indexes_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Deletion of Index failed\n");
  state = S3ClovisKVSWriterOpState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}


void S3ClovisKVSWriter::put_keyval(std::string index_name, std::string key, std::string  val, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "index_name = %s, key = %s and value = %s\n", index_name.c_str(), key.c_str(), val.c_str());
  int rc = 0;
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::put_keyval_successful, this), std::bind( &S3ClovisKVSWriter::put_keyval_failed, this)));

  // Only one key value passed
  writer_context->init_kvs_write_op_ctx(1);

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();
  struct s3_clovis_kvs_op_context *kvs_ctx = writer_context->get_clovis_kvs_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_ctx->cbs->oop_executed = NULL;
  idx_ctx->cbs->oop_stable = s3_clovis_op_stable;
  idx_ctx->cbs->oop_failed = s3_clovis_op_failed;

  S3UriToMeroOID(index_name.c_str(), &id);

  set_up_key_value_store(kvs_ctx, key, val);

  s3_clovis_api->clovis_idx_init(idx_ctx->idx, &clovis_container.co_realm, &id);
  rc = s3_clovis_api->clovis_idx_op(idx_ctx->idx,
                                    M0_CLOVIS_IC_PUT,
                                    kvs_ctx->keys,
                                    kvs_ctx->values,
                                    &(idx_ctx->ops[0]));
  if(rc != 0) {
    s3_log(S3_LOG_DEBUG, "m0_clovis_idx_op failed\n");
  }
  else {
    s3_log(S3_LOG_DEBUG, "m0_clovis_idx_op suceeded\n");
  }

  idx_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);

  writer_context->start_timer_for("put_keyval");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::put_keyval_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSWriterOpState::saved;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::put_keyval_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Writing of key value failed\n");
  state = S3ClovisKVSWriterOpState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_keyval(std::string index_name, std::string key, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  std::vector<std::string> keys;
  keys.push_back(key);

  delete_keyval(index_name, keys, on_success, on_failed);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_keyval(std::string index_name, std::vector<std::string> keys, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  int rc;
  s3_log(S3_LOG_DEBUG, "index_name = %s\n", index_name.c_str());
  for(auto key : keys) {
    s3_log(S3_LOG_DEBUG, "key = %s\n", key.c_str());
  }

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::delete_keyval_successful, this), std::bind( &S3ClovisKVSWriter::delete_keyval_failed, this)));

  // Only one key value passed
  writer_context->init_kvs_write_op_ctx(keys.size());

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();
  struct s3_clovis_kvs_op_context *kvs_ctx = writer_context->get_clovis_kvs_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_ctx->cbs->oop_executed = NULL;
  idx_ctx->cbs->oop_stable = s3_clovis_op_stable;
  idx_ctx->cbs->oop_failed = s3_clovis_op_failed;

  S3UriToMeroOID(index_name.c_str(), &id);

  int i = 0;
  for(auto key : keys) {
    kvs_ctx->keys->ov_vec.v_count[i] = key.length();
    kvs_ctx->keys->ov_buf[i] = calloc(1, key.length());
    memcpy(kvs_ctx->keys->ov_buf[i], (void*)key.c_str(), key.length());
    ++i;
  }

  s3_clovis_api->clovis_idx_init(idx_ctx->idx, &clovis_container.co_realm, &id);
  rc = s3_clovis_api->clovis_idx_op(idx_ctx->idx, M0_CLOVIS_IC_DEL, kvs_ctx->keys, NULL, &(idx_ctx->ops[0]));
  if( rc != 0 ) {
    s3_log(S3_LOG_ERROR, "m0_clovis_idx_op failed\n");
  }
  else {
    s3_log(S3_LOG_DEBUG, "m0_clovis_idx_op suceeded\n");
  }

  idx_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);

  writer_context->start_timer_for("delete_keyval");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_keyval_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::delete_keyval_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSWriterOpState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSWriter::set_up_key_value_store(struct s3_clovis_kvs_op_context *kvs_ctx, std::string key, std::string val) {

  // TODO - clean up these buffers
  s3_log(S3_LOG_DEBUG, "Entering\n");
  kvs_ctx->keys->ov_vec.v_count[0] = key.length();
  kvs_ctx->keys->ov_buf[0] = (char *)malloc(key.length());
  memcpy(kvs_ctx->keys->ov_buf[0], (void *)key.c_str(), key.length());

  kvs_ctx->values->ov_vec.v_count[0] = val.length();
  kvs_ctx->values->ov_buf[0] = (char *)malloc(val.length());
  memcpy(kvs_ctx->values->ov_buf[0], (void *)val.c_str(), val.length());

  s3_log(S3_LOG_DEBUG, "Keys and value in clovis buffer\n");
  s3_log(S3_LOG_DEBUG, "kvs_ctx->keys->ov_buf[0] = %s\n", std::string((char*)kvs_ctx->keys->ov_buf[0], kvs_ctx->keys->ov_vec.v_count[0]).c_str());
  s3_log(S3_LOG_DEBUG, "kvs_ctx->vals->ov_buf[0] = %s\n",std::string((char*)kvs_ctx->values->ov_buf[0], kvs_ctx->values->ov_vec.v_count[0]).c_str());
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}


#include <unistd.h>

#include "s3_common.h"

#include "s3_clovis_rw_common.h"
#include "s3_clovis_config.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_scope     clovis_uber_scope;
extern struct m0_clovis_container clovis_container;

S3ClovisKVSWriter::S3ClovisKVSWriter(std::shared_ptr<S3RequestObject> req) : request(req),state(S3ClovisKVSWriterOpState::start) {
  printf("S3ClovisKVSWriter created\n");
}

S3ClovisKVSWriter::~S3ClovisKVSWriter() {
  printf("S3ClovisKVSWriter deleted.\n");
}

void S3ClovisKVSWriter::create_index(std::string index_name, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("S3ClovisKVSWriter::create_index called with index_name = %s\n", index_name.c_str());
  int                     rc = 0;

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::create_index_successful, this), std::bind( &S3ClovisKVSWriter::create_index_failed, this)));

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();

  idx_ctx->cbs->ocb_arg = (void *)writer_context.get();
  idx_ctx->cbs->ocb_executed = NULL;
  idx_ctx->cbs->ocb_stable = s3_clovis_op_stable;
  idx_ctx->cbs->ocb_failed = s3_clovis_op_failed;


  S3UriToMeroOID(index_name.c_str(), &id);

  m0_clovis_idx_init(idx_ctx->idx, &clovis_uber_scope, &id);
  rc = m0_clovis_entity_create(&idx_ctx->idx->in_entity, &(idx_ctx->ops[0]));
  if(rc != 0) {
    printf("m0_clovis_entity_create failed\n");
  }

   m0_clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);
   m0_clovis_op_launch(idx_ctx->ops, 1);
}

void S3ClovisKVSWriter::create_index_successful() {
  printf("S3ClovisKVSWriter::create_index_successful called\n");
  state = S3ClovisKVSWriterOpState::saved;
  this->handler_on_success();
}

void S3ClovisKVSWriter::create_index_failed() {
  printf("S3ClovisKVSWriter::create_index_failed called\n");
  if (writer_context->get_errno() == -EEXIST) {
    state = S3ClovisKVSWriterOpState::exists;
  } else {
    state = S3ClovisKVSWriterOpState::failed;
  }
  this->handler_on_failed();
}

void S3ClovisKVSWriter::put_keyval(std::string index_name, std::string key, std::string  val, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("S3ClovisKVSWriter::put_keyval called with index_name = %s and key = %s and value = %s\n", index_name.c_str(), key.c_str(), val.c_str());

  int rc = 0;
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisKVSWriterContext(request, std::bind( &S3ClovisKVSWriter::put_keyval_successful, this), std::bind( &S3ClovisKVSWriter::put_keyval_failed, this)));

  // Only one key value passed
  writer_context->init_kvs_write_op_ctx(1);

  struct s3_clovis_idx_op_context *idx_ctx = writer_context->get_clovis_idx_op_ctx();
  struct s3_clovis_kvs_op_context *kvs_ctx = writer_context->get_clovis_kvs_op_ctx();

  idx_ctx->cbs->ocb_arg = (void *)writer_context.get();
  idx_ctx->cbs->ocb_executed = NULL;
  idx_ctx->cbs->ocb_stable = s3_clovis_op_stable;
  idx_ctx->cbs->ocb_failed = s3_clovis_op_failed;

  S3UriToMeroOID(index_name.c_str(), &id);

  set_up_key_value_store(kvs_ctx, key, val);

  m0_clovis_idx_init(idx_ctx->idx, &clovis_container.co_scope, &id);

  rc = m0_clovis_idx_op(idx_ctx->idx, M0_CLOVIS_IC_PUT, kvs_ctx->keys, kvs_ctx->values, &(idx_ctx->ops[0]));
  if(rc  != 0) {
    printf("m0_clovis_idx_op failed\n");
  }
  else {
    printf("m0_clovis_idx_op suceeded\n");
  }

  m0_clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);
  m0_clovis_op_launch(idx_ctx->ops, 1);
}

void S3ClovisKVSWriter::put_keyval_successful() {
  printf("S3ClovisKVSWriter::put_keyval_successful called\n");
  state = S3ClovisKVSWriterOpState::saved;
  this->handler_on_success();
}

void S3ClovisKVSWriter::put_keyval_failed() {
  printf("S3ClovisKVSWriter::put_keyval_failed called\n");
  state = S3ClovisKVSWriterOpState::failed;
  this->handler_on_failed();
}

void S3ClovisKVSWriter::set_up_key_value_store(struct s3_clovis_kvs_op_context *kvs_ctx, std::string key, std::string val) {

  // TODO - clean up these buffers

  kvs_ctx->keys->ov_vec.v_count[0] = key.length();
  kvs_ctx->keys->ov_buf[0] = (char *)malloc(key.length());
  memcpy(kvs_ctx->keys->ov_buf[0], (void *)key.c_str(), key.length());

  kvs_ctx->values->ov_vec.v_count[0] = val.length();
  kvs_ctx->values->ov_buf[0] = (char *)malloc(val.length());
  memcpy(kvs_ctx->values->ov_buf[0], (void *)val.c_str(), val.length());

  printf("Keys and value in clovis buffer\n");
  printf("kvs_ctx->keys->ov_buf[0] = %s\n", std::string((char*)kvs_ctx->keys->ov_buf[0], kvs_ctx->keys->ov_vec.v_count[0]).c_str());
  printf("kvs_ctx->vals->ov_buf[0] = %s\n",std::string((char*)kvs_ctx->values->ov_buf[0], kvs_ctx->values->ov_vec.v_count[0]).c_str());
}

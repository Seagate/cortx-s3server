
#include "s3_bucket_metadata.h"

// This is run on main thread.
extern "C" void bucket_rw_done_on_main_thread(evutil_socket_t, short events, void *user_data) {
  S3BucketMetadataRWContext *context = (S3BucketMetadataRWContext*) user_data;
  context->handler()();  // Invoke the handler.
  delete context;
}

// Run within clovis thread.
extern "C" void bucket_rw_done_on_clovis_thread(S3AsyncOpStatus op_status, struct s3_e_kv_pair kv, void* context)
{
  struct S3BucketMetadataRWContext *context = (struct S3BucketMetadataRWContext *)op->op_cbs->ocb_arg;

  if (op_status == S3AsyncOpStatus::success) {
    context->set_op_status(S3AsyncOpStatus::success, "Successfull");
    if (context->operation_type() == S3BucketMetadataOpType::r) {
      context->get_metadata()->from_json(std::string(kv.value, kv.value_length));
    } else {
      context->get_metadata()->saved();
    }
  } else {
    context->set_op_status(S3AsyncOpStatus::failed, "Some error message.");
  }

  S3PostToMainLoop(context->get_request(), context)(bucket_rw_done_on_main_thread);
}

S3BucketMetadata::S3BucketMetadata(S3RequestObject* req) : request(req) {
    this->bucket_name = request->bucket_name();
    this->state = S3BucketMetadataState::empty;
}

void S3BucketMetadata::saved() {
  state = S3BucketMetadataState::saved;
}

void S3BucketMetadata::async_load(std::function<void(void)> handler) {
  // Load from clovis.
  S3BucketMetadataRWContext context = new S3BucketMetadataRWContext(request, handler, this, S3BucketMetadataOpType::r);

  // Mark absent as we initiate fetch, in case it fails to load due to missing.
  state = S3BucketMetadataState::absent;
  // Clovis API to read bucket data
  s3_get_key_val(request->user_name(), bucket_name, bucket_rw_done_on_clovis_thread, context);
}

void S3BucketMetadata::async_save(std::function<void(void)> handler) {
  // Load from clovis.
  S3BucketMetadataRWContext context = new S3BucketMetadataRWContext(request, handler, this, S3BucketMetadataOpType::w);

  // Mark absent as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::absent;
  // Clovis API to write bucket data
  s3_put_key_val(request->user_name(), bucket_name, this->to_json(), bucket_rw_done_on_clovis_thread, context);
}

// AWS recommends that all bucket names comply with DNS naming convention
// See Bucket naming restrictions in above link.
void S3BucketMetadata::validate_bucket_name() {
  // TODO
}

void S3BucketMetadata::validate() {
  // TODO
}

void S3BucketMetadata::validate_bucket_name() {
  // TODO
}

// Streaming to json
std::string S3BucketMetadata::to_json() {
  // TODO
}

void S3BucketMetadata::from_json(std::string content) {
  // TODO
  state = S3BucketMetadataState::present;
}

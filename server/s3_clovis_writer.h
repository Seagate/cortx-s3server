
#include "s3_request_object.h"
#include "s3_asyncop_context_base.h"

class S3ClovisWriterContext : public S3AsyncOpContextBase {
  struct s3_clovis_op_context clovis_op_ctx;
  S3ClovisWriter *clovis_writer;
public:
  S3ClovisWriterContext(S3RequestObject *req, std::function<void(void)> handler, S3ClovisWriter *writer, struct s3_clovis_op_context op_ctx) : S3AsyncOpContextBase(req, handler), clovis_writer(writer), clovis_op_ctx(op_ctx) {}

  S3ClovisWriter* clovis_writer() {
    return clovis_writer;
  }

  struct s3_clovis_op_context* clovis_op_ctx() {
    return &clovis_op_ctx;
  }
};

enum class S3ClovisWriterOpState {
  failed,
  start,
  created,
  saved,
}

class S3ClovisWriter {
  S3RequestObject* request;
  std::function<void()> handler;
  S3ClovisWriterOpState state;
public:
  S3ClovisWriter(S3RequestObject* req);

  void advance_state();
  void mark_failed();

  // Async save operation.
  void save();
}

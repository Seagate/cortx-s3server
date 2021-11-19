#include <json/json.h>
#include "s3_bucket_counters.h"
#include "s3_log.h"

extern struct s3_motr_idx_layout bucket_object_count_index_layout;

S3BucketObjectCounter::S3BucketObjectCounter(
    std::shared_ptr<S3RequestObject> request, std::string bkt_name,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  req = std::move(request);
  request_id = req->get_request_id();
  bucket_name = bkt_name;

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (kv_reader_factory) {
    motr_kv_reader_factory = std::move(kv_reader_factory);
  } else {
    motr_kv_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

std::string generate_unique_id() {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);

  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
  return "1";
}

void S3BucketObjectCounter::save(std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // object_list_index_layout should be set before using this method
  // assert(non_zero(bucket_object_count_index_layout.oid));

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(req, s3_motr_api);
  }

  std::string key = bucket_name + "/" + generate_unique_id();

  motr_kv_writer->put_keyval(
      bucket_object_count_index_layout, key, this->to_json(),
      std::bind(&S3BucketObjectCounter::save_counters_successful, this),
      std::bind(&S3BucketObjectCounter::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::save_counters_successful() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  this->handler_on_success();
}

void S3BucketObjectCounter::save_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  this->handler_on_failed();
}

std::string S3BucketObjectCounter::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");
  Json::Value root;
  root["Object_count"] = 1;
  root["Total_size"] = 1;
  root["Degraded_count"] = 1;

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}
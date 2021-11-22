#include <json/json.h>
#include "s3_bucket_counters.h"
#include "s3_log.h"

extern struct s3_motr_idx_layout bucket_object_count_index_layout;

std::unique_ptr<S3BucketCapacityCache> S3BucketCapacityCache::singleton;

void S3BucketCapacityCache::update_bucket_capacity(
    const S3BucketMetadata& src, int count_objects_increment,
    intmax_t count_bytes_increment, std::function<void()> on_success,
    std::function<void()> on_failure) {

  get_instance()->update_impl(src, count_objects_increment,
                              count_bytes_increment, on_success, on_failure);
}

void S3BucketCapacityCache::update_impl(const S3BucketMetadata& src,
                                        int count_objects_increment,
                                        intmax_t count_bytes_increment,
                                        std::function<void()> on_success,
                                        std::function<void()> on_failure) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(),
         "Updating count+=%d, capacity+=%" PRIdMAX, count_objects_increment,
         count_bytes_increment);

  // TBD: do the update

  on_success();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

std::string generate_unique_id() {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);

  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
  return "1";
}

S3BucketObjectCounter::S3BucketObjectCounter(
    std::shared_ptr<S3RequestObject> request,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  req = std::move(request);
  request_id = req->get_request_id();
  bucket_name = req->get_bucket_name();
  key = bucket_name + "/" + generate_unique_id();
  inc_object_count = 0;
  inc_total_size = 0;
  inc_degraded_count = 0;
  is_cache_created = false;

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

void S3BucketObjectCounter::save(std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // object_list_index_layout should be set before using this method
  // assert(non_zero(bucket_object_count_index_layout.oid));

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  if (!is_cache_created) {
    if (!motr_kv_reader) {
      motr_kv_reader =
          motr_kv_reader_factory->create_motr_kvs_reader(req, s3_motr_api);
    }

    motr_kv_reader->get_keyval(
        bucket_object_count_index_layout, key,
        std::bind(&S3BucketObjectCounter::load_successful, this),
        std::bind(&S3BucketObjectCounter::load_failed, this));
  } else {
    save_bucket_counters();
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::load_successful() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (this->from_json(motr_kv_reader->get_value()) != 0) {

    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 ":%" SCNx64 ", Key = %s, Value = %s\n",
           bucket_object_count_index_layout.oid.u_hi,
           bucket_object_count_index_layout.oid.u_lo, key.c_str(),
           motr_kv_reader->get_value().c_str());

    this->handler_on_failed();
  }

  is_cache_created = true;
  save_bucket_counters();

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::load_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  saved_object_count = 0;
  saved_total_size = 0;
  saved_degraded_count = 0;
  is_cache_created = true;
  save_bucket_counters();

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::save_bucket_counters() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(req, s3_motr_api);
  }

  motr_kv_writer->put_keyval(
      bucket_object_count_index_layout, key, this->to_json(),
      std::bind(&S3BucketObjectCounter::save_counters_successful, this),
      std::bind(&S3BucketObjectCounter::save_metadata_failed, this));

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::save_counters_successful() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // This is so that we dont have to call
  // getkeyval everytime. We only do it once
  // then we maintain things in memory.
  // this may be time efficient.
  saved_object_count += inc_object_count;
  saved_total_size += inc_total_size;
  saved_degraded_count += inc_degraded_count;

  inc_object_count = 0;
  inc_total_size = 0;
  inc_degraded_count = 0;

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
  root[OBJECT_COUNT] =
      Json::Value((Json::Value::UInt64)(saved_object_count + inc_object_count));
  root[TOTAL_SIZE] =
      Json::Value((Json::Value::UInt64)(saved_total_size + inc_total_size));
  root[DEGRADED_COUNT] = Json::Value(
      (Json::Value::UInt64)(saved_degraded_count + inc_degraded_count));

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int S3BucketObjectCounter::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "Called with content [%s]\n",
         content.c_str());
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed\n");
    return -1;
  }

  saved_object_count = newroot[OBJECT_COUNT].asUInt64();
  saved_total_size = newroot[TOTAL_SIZE].asUInt64();
  saved_degraded_count = newroot[DEGRADED_COUNT].asUInt64();

  s3_log(S3_LOG_INFO, request_id,
         "%s Load complete. saved_object_count = %lu, saved_total_size = %lu, "
         "saved_degraded_count = %lu\n",
         __func__, saved_object_count, saved_total_size, saved_degraded_count);

  s3_log(S3_LOG_DEBUG, request_id, "[%s] Exit\n", __func__);
  return 0;
}

void S3BucketObjectCounter::add_inc_object_count(int64_t obj_count) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  inc_object_count = obj_count;
  s3_log(S3_LOG_INFO, request_id, "%s inc_object_count = %lu\n", __func__,
         inc_object_count);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::add_inc_size(int64_t size) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  inc_total_size = size;
  s3_log(S3_LOG_INFO, request_id, "%s inc_total_size = %lu\n", __func__,
         inc_total_size);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3BucketObjectCounter::add_inc_degraded_count(int64_t degraded_count) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  inc_degraded_count = degraded_count;
  s3_log(S3_LOG_INFO, request_id, "%s inc_degraded_count = %lu\n", __func__,
         inc_degraded_count);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

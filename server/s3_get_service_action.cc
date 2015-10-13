
#include <string>

#include "s3_clovis_config.h"
#include "s3_get_service_action.h"
#include "s3_bucket_metadata.h"
#include "s3_error_codes.h"

S3GetServiceAction::S3GetServiceAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), fetch_successful(false) {
  setup_steps();
  bucket_list.set_owner_name(request->get_user_name());
  bucket_list.set_owner_id(request->get_user_id());
}

void S3GetServiceAction::setup_steps(){
  add_task(std::bind( &S3GetServiceAction::get_next_buckets, this ));
  add_task(std::bind( &S3GetServiceAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetServiceAction::get_next_buckets() {
  printf("Called S3GetServiceAction::get_next_buckets\n");
  size_t count = S3ClovisConfig::get_instance()->get_clovis_idx_fetch_count();

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->next_keyval(get_account_user_index_name(), last_key, count, std::bind( &S3GetServiceAction::get_next_buckets_successful, this), std::bind( &S3GetServiceAction::get_next_buckets_failed, this));
}

void S3GetServiceAction::get_next_buckets_successful() {
  printf("Called S3GetServiceAction::get_next_buckets_successful\n");
  auto& kvps = clovis_kv_reader->get_key_values();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    auto bucket = std::make_shared<S3BucketMetadata>(request);
    bucket->from_json(kv.second);
    bucket_list.add_bucket(bucket);
    if (--length == 0) {
      // this is the last element returned.
      last_key = kv.first;
    }
  }
  // We ask for more if there is any.
  size_t count_we_requested = S3ClovisConfig::get_instance()->get_clovis_idx_fetch_count();

  if (kvps.size() < count_we_requested) {
    // Go ahead and respond.
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    get_next_buckets();
  }
}

void S3GetServiceAction::get_next_buckets_failed() {
  printf("Called S3GetServiceAction::get_next_buckets_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    fetch_successful = true;  // With no entries.
  } else {
    fetch_successful = false;
  }
  send_response_to_s3_client();
}

void S3GetServiceAction::send_response_to_s3_client() {
  printf("Called S3GetServiceAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (fetch_successful) {
    std::string& response_xml = bucket_list.get_xml();
    std::string content_len_key("Content-Length");
    request->set_out_header_value(content_len_key, std::to_string(response_xml.length()));
    printf("Bucket list response_xml = %s\n", response_xml.c_str());
    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
}

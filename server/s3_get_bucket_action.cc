
#include <string>

#include "s3_clovis_config.h"
#include "s3_get_bucket_action.h"
#include "s3_object_metadata.h"
#include "s3_error_codes.h"

S3GetBucketAction::S3GetBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), return_list_size(0), fetch_successful(false) {
  setup_steps();
  object_list.set_bucket_name(request->get_bucket_name());
  request_prefix = request->get_query_string_value("prefix");
  object_list.set_request_prefix(request_prefix);
  printf("prefix = %s\n", request_prefix.c_str());

  request_delimiter = request->get_query_string_value("delimiter");
  object_list.set_request_delimiter(request_delimiter);
  printf("delimiter = %s\n", request_delimiter.c_str());

  request_marker_key = request->get_query_string_value("marker");
  object_list.set_request_marker_key(request_marker_key);
  printf("request_marker_key = %s\n", request_marker_key.c_str());

  last_key = request_marker_key;  // as requested by user
  std::string max_k = request->get_query_string_value("max-keys");
  if (max_k.empty()) {
    max_keys = 1000;
    object_list.set_max_keys("1000");
  } else {
    max_keys = std::stoul(max_k);
    object_list.set_max_keys(max_k);
  }
  printf("max-keys = %s\n", max_k.c_str());
  // TODO request param validations
}

void S3GetBucketAction::setup_steps(){
  add_task(std::bind( &S3GetBucketAction::get_next_objects, this ));
  add_task(std::bind( &S3GetBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetBucketAction::get_next_objects() {
  printf("Called S3GetBucketAction::get_next_objects\n");
  size_t count = S3ClovisConfig::get_instance()->get_clovis_idx_fetch_count();

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->next_keyval(get_bucket_index_name(), last_key, count, std::bind( &S3GetBucketAction::get_next_objects_successful, this), std::bind( &S3GetBucketAction::get_next_objects_failed, this));
}

void S3GetBucketAction::get_next_objects_successful() {
  printf("Called S3GetBucketAction::get_next_objects_successful\n");
  auto& kvps = clovis_kv_reader->get_key_values();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    printf("Read Object = %s\n", kv.first.c_str());
    auto object = std::make_shared<S3ObjectMetadata>(request);
    size_t delimiter_pos = std::string::npos;
    if (request_prefix.empty() && request_delimiter.empty()) {
      object->from_json(kv.second);
      object_list.add_object(object);
    } else if (!request_prefix.empty() && request_delimiter.empty()) {
      // Filter out by prefix
      if (kv.first.find(request_prefix) == 0) {
        object->from_json(kv.second);
        object_list.add_object(object);
      }
    } else if (request_prefix.empty() && !request_delimiter.empty()) {
      delimiter_pos = kv.first.find(request_delimiter);
      if (delimiter_pos == std::string::npos) {
        object->from_json(kv.second);
        object_list.add_object(object);
      } else {
        // Roll up
        printf("Delimiter %s found at pos %d in string %s\n", request_delimiter.c_str(), delimiter_pos, kv.first.c_str());
        object_list.add_common_prefix(kv.first.substr(0, delimiter_pos + 1));
      }
    } else {
      // both prefix and delimiter are not empty
      bool prefix_match = (kv.first.find(request_prefix) == 0) ? true : false;
      if (prefix_match) {
        delimiter_pos = kv.first.find(request_delimiter, request_prefix.length());
        if (delimiter_pos == std::string::npos) {
          object->from_json(kv.second);
          object_list.add_object(object);
        } else {
          printf("Delimiter %s found at pos %d in string %s\n", request_delimiter.c_str(), delimiter_pos, kv.first.c_str());
          object_list.add_common_prefix(kv.first.substr(0, delimiter_pos + 1));
        }
      } // else no prefix match, filter it out
    }

    return_list_size++;
    if (--length == 0 || return_list_size == max_keys) {
      // this is the last element returned or we reached limit requested
      last_key = kv.first;
      break;
    }
  }
  // We ask for more if there is any.
  size_t count_we_requested = S3ClovisConfig::get_instance()->get_clovis_idx_fetch_count();

  if ((return_list_size == max_keys) || (kvps.size() < count_we_requested)) {
    // Go ahead and respond.
    if (return_list_size == max_keys) {
      object_list.set_response_is_truncated(true);
      object_list.set_next_marker_key(last_key);
    }
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    get_next_objects();
  }
}

void S3GetBucketAction::get_next_objects_failed() {
  printf("Called S3GetBucketAction::get_next_objects_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    fetch_successful = true;  // With no entries.
  } else {
    fetch_successful = false;
  }
  send_response_to_s3_client();
}

void S3GetBucketAction::send_response_to_s3_client() {
  printf("Called S3GetBucketAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (fetch_successful) {
    std::string& response_xml = object_list.get_xml();
    std::string content_len_key("Content-Length");
    request->set_out_header_value(content_len_key, std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    printf("Object list response_xml = %s\n", response_xml.c_str());
    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
}

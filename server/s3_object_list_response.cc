
#include "s3_object_list_response.h"

S3ObjectListResponse::S3ObjectListResponse() : request_prefix(""), request_delimiter(""), request_marker_key(""), max_keys(""), response_is_truncated(false), next_marker_key(""), response_xml("") {
  printf("S3ObjectListResponse created\n");
  object_list.clear();
}

void S3ObjectListResponse::set_bucket_name(std::string name) {
  bucket_name = name;
}

void S3ObjectListResponse::set_request_prefix(std::string prefix) {
  request_prefix = prefix;
}

void S3ObjectListResponse::set_request_delimiter(std::string delimiter) {
  request_delimiter = delimiter;
}

void S3ObjectListResponse::set_request_marker_key(std::string marker) {
  request_marker_key = marker;
}

void S3ObjectListResponse::set_max_keys(std::string count) {
  max_keys = count;
}

void S3ObjectListResponse::set_response_is_truncated(bool flag) {
  response_is_truncated = flag;
}

void S3ObjectListResponse::set_next_marker_key(std::string next) {
  next_marker_key = next;
}

void S3ObjectListResponse::add_object(std::shared_ptr<S3ObjectMetadata> object) {
  object_list.push_back(object);
}

std::string& S3ObjectListResponse::get_xml() {
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  response_xml += "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
  response_xml += "<Name>" + bucket_name + "</Name>\n"
                  "<Prefix>" + request_prefix + "</Prefix>\n"
                  "<Delimiter>" + request_delimiter + "</Delimiter>\n"
                  "<Marker>" + request_marker_key + "</Marker>\n"
                  "<MaxKeys>" + max_keys + "</MaxKeys>\n"
                  "<NextMarker>" + (response_is_truncated ? next_marker_key : "") + "</NextMarker>\n"
                  "<IsTruncated>" + (response_is_truncated ? "true" : "false") + "</IsTruncated>\n";

  for (auto&& object : object_list) {
    response_xml += "<Contents>\n"
                    "  <Key>" + object->get_object_name() + "</Key>\n"
                    "  <LastModified>" + object->get_last_modified() + "</LastModified>\n"
                    "  <ETag>" + object->get_md5() + "</ETag>\n"
                    "  <Size>" + object->get_content_length_str() + "</Size>\n"
                    "  <StorageClass>" + object->get_storage_class() + "</StorageClass>\n"
                    "  <Owner>\n"
                    "    <ID>" + object->get_user_id() + "</ID>\n"
                    "    <DisplayName>" + object->get_user_name() + "</DisplayName>\n"
                    "  </Owner>\n"
                    "</Contents>";
  }
  response_xml += "</ListBucketResult>\n";

  return response_xml;
}

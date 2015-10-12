
#include "s3_service_list_response.h"

S3ServiceListResponse::S3ServiceListResponse() {
  printf("S3ServiceListResponse created\n");
  bucket_list.clear();
}

void S3ServiceListResponse::set_owner_name(std::string name) {
  owner_name = name;
}

void S3ServiceListResponse::set_owner_id(std::string id) {
  owner_id = id;
}

void S3ServiceListResponse::add_bucket(std::shared_ptr<S3BucketMetadata> bucket) {
  bucket_list.push_back(bucket);
}

std::string& S3ServiceListResponse::get_xml() {
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  response_xml += "<ListAllMyBucketsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01\">\n";
  response_xml += "<Owner>\n"
                  "  <ID>" + owner_id + "</ID>\n"
                  "  <DisplayName>" + owner_name + "</DisplayName>\n"
                  "</Owner>\n";

  response_xml += "<Buckets>\n";
  for (auto&& bucket : bucket_list) {
    response_xml += "<Bucket>\n"
                    "  <Name>" + bucket->get_bucket_name() + "</Name>\n"
                    "  <CreationDate>" + bucket->get_creation_time() + "</CreationDate>\n"
                    "</Bucket>";
  }
  response_xml += "</Buckets>\n";
  response_xml += "</ListAllMyBucketsResult>\n";

  return response_xml;
}

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
 * Original creation date: 1-Oct-2015
 */

#include "s3_object_list_response.h"
#include "s3_log.h"

S3ObjectListResponse::S3ObjectListResponse()
    : request_prefix(""),
      request_delimiter(""),
      request_marker_key(""),
      request_marker_uploadid(""),
      max_keys(""),
      response_is_truncated(false),
      next_marker_key(""),
      response_xml(""),
      max_uploads(""),
      next_marker_uploadid("") {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  object_list.clear();
  part_list.clear();
}

void S3ObjectListResponse::set_bucket_name(std::string name) {
  bucket_name = name;
}

void S3ObjectListResponse::set_object_name(std::string name) {
  object_name = name;
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

void S3ObjectListResponse::set_request_marker_uploadid(std::string marker) {
  request_marker_uploadid = marker;
}

void S3ObjectListResponse::set_max_keys(std::string count) { max_keys = count; }

void S3ObjectListResponse::set_max_uploads(std::string upload_count) {
  max_uploads = upload_count;
}

void S3ObjectListResponse::set_max_parts(std::string part_count) {
  max_parts = part_count;
}

void S3ObjectListResponse::set_response_is_truncated(bool flag) {
  response_is_truncated = flag;
}

void S3ObjectListResponse::set_next_marker_key(std::string next) {
  next_marker_key = next;
}

void S3ObjectListResponse::set_next_marker_uploadid(std::string next) {
  next_marker_uploadid = next;
}

std::string& S3ObjectListResponse::get_object_name() { return object_name; }

void S3ObjectListResponse::add_object(
    std::shared_ptr<S3ObjectMetadata> object) {
  object_list.push_back(object);
}

unsigned int S3ObjectListResponse::size() { return object_list.size(); }

unsigned int S3ObjectListResponse::common_prefixes_size() {
  return common_prefixes.size();
}

void S3ObjectListResponse::add_part(std::shared_ptr<S3PartMetadata> part) {
  part_list[strtoul(part->get_part_number().c_str(), NULL, 0)] = part;
}

void S3ObjectListResponse::add_common_prefix(std::string common_prefix) {
  common_prefixes.insert(common_prefix);
}

void S3ObjectListResponse::set_user_id(std::string userid) { user_id = userid; }

void S3ObjectListResponse::set_user_name(std::string username) {
  user_name = username;
}

void S3ObjectListResponse::set_account_id(std::string accountid) {
  account_id = accountid;
}

void S3ObjectListResponse::set_account_name(std::string accountname) {
  account_name = accountname;
}

std::string& S3ObjectListResponse::get_account_id() { return account_id; }

std::string& S3ObjectListResponse::get_account_name() { return account_name; }

void S3ObjectListResponse::set_storage_class(std::string stor_class) {
  storage_class = stor_class;
}

std::string& S3ObjectListResponse::get_user_id() { return user_id; }

std::string& S3ObjectListResponse::get_user_name() { return user_name; }

std::string& S3ObjectListResponse::get_storage_class() { return storage_class; }

void S3ObjectListResponse::set_upload_id(std::string uploadid) {
  upload_id = uploadid;
}

std::string& S3ObjectListResponse::get_upload_id() { return upload_id; }

std::string& S3ObjectListResponse::get_xml() {
  // clang-format off
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  response_xml += "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
  response_xml += "<Name>" + bucket_name + "</Name>\n"
                  "<Prefix>" + request_prefix + "</Prefix>\n"
                  "<Delimiter>" + request_delimiter + "</Delimiter>\n"
                  "<Marker>" + request_marker_key + "</Marker>\n"
                  "<MaxKeys>" + max_keys + "</MaxKeys>\n"
                  "<NextMarker>" + next_marker_key + "</NextMarker>\n"
                  "<IsTruncated>" + (response_is_truncated ? "true" : "false") + "</IsTruncated>\n";

  for (auto&& object : object_list) {
    response_xml += "<Contents>\n"
                    "  <Key>" + object->get_object_name() + "</Key>\n"
                    "  <LastModified>" + object->get_last_modified_iso() + "</LastModified>\n"
                    "  <ETag>" + object->get_md5() + "</ETag>\n"
                    "  <Size>" + object->get_content_length_str() + "</Size>\n"
                    "  <StorageClass>" + object->get_storage_class() + "</StorageClass>\n"
                    "  <Owner>\n"
                    "    <ID>" + object->get_user_id() + "</ID>\n"
                    "    <DisplayName>" + object->get_user_name() + "</DisplayName>\n"
                    "  </Owner>\n"
                    "</Contents>";
  }

  for (auto&& prefix : common_prefixes) {
    response_xml += "<CommonPrefixes>\n"
                    "  <Prefix>" + prefix + "</Prefix>\n"
        "</CommonPrefixes>";
  }

  response_xml += "</ListBucketResult>\n";
  // clang-format on
  return response_xml;
}

std::string& S3ObjectListResponse::get_multiupload_xml() {
  // clang-format off
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  response_xml += "<ListMultipartUploadsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
  response_xml += "<Bucket>" + bucket_name + "</Bucket>"
                  "<KeyMarker>" + request_marker_key + "</KeyMarker>"
                  "<UploadIdMarker>" + request_marker_uploadid + "</UploadIdMarker>"
                  "<NextKeyMarker>" + next_marker_key + "</NextKeyMarker>"
                  "<NextUploadIdMarker>" + next_marker_uploadid + "</NextUploadIdMarker>"
                  "<MaxUploads>" + max_uploads + "</MaxUploads>\n"
                  "<IsTruncated>" + (response_is_truncated ? "true" : "false") + "</IsTruncated>\n";

  for (auto&& object : object_list) {
    response_xml += "<Upload>\n"
                    "  <Key>" + object->get_object_name() + "</Key>\n"
                    "  <UploadId>" + object->get_upload_id() + "</UploadId>\n"
                    "  <Initiator>\n"
                    "    <ID>" + object->get_user_id() + "</ID>\n"
                    "    <DisplayName>" + object->get_user_name() + "</DisplayName>\n"
                    "  </Initiator>\n"
                    "  <Owner>\n"
                    "    <ID>" + object->get_user_id() + "</ID>\n"
                    "    <DisplayName>" + object->get_user_name() + "</DisplayName>\n"
                    "  </Owner>\n"
                    "  <StorageClass>" + object->get_storage_class() + "</StorageClass>\n"
                    "  <Initiated>" + object->get_last_modified_iso() + "</Initiated>\n"
                    "</Upload>\n";
  }

  for (auto&& prefix : common_prefixes) {
    response_xml += "<CommonPrefixes>\n"
                    "  <Prefix>" + prefix + "</Prefix>\n"
                    "</CommonPrefixes>";
  }

  response_xml += "</ListMultipartUploadsResult>\n";
  // clang-format on
  return response_xml;
}

std::string& S3ObjectListResponse::get_multipart_xml() {
  // clang-format off
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  response_xml += "<ListPartsResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
  response_xml += "  <Bucket>" + bucket_name + "</Bucket>\n"
                  "  <Key>" + get_object_name() + "</Key>\n"
                  "  <UploadID>" + get_upload_id() + "</UploadID>\n"
                  "  <Initiator>\n"
                  "      <ID>" + get_user_id() + "</ID>\n"
                  "      <DisplayName>" + get_user_name() + "</DisplayName>\n"
                  "  </Initiator>\n"
                  "  <Owner>\n"
                  "    <ID>" + get_account_id() + "</ID>\n"
                  "    <DisplayName>" + get_account_name() + "</DisplayName>\n"
                  "  </Owner>\n"
                  "  <StorageClass>" + get_storage_class() + "</StorageClass>\n"
                  "  <PartNumberMarker>" + request_marker_key + "</PartNumberMarker>\n"
                  "  <NextPartNumberMarker>" + (next_marker_key.empty() ? "0" : next_marker_key) + "</NextPartNumberMarker>\n"
                  "  <MaxParts>" + max_parts + "</MaxParts>\n"
                  "  <IsTruncated>" + (response_is_truncated ? "true" : "false") + "</IsTruncated>\n";

  for (auto&& part : part_list) {
    response_xml += "  <Part>\n"
                    "  <PartNumber>" + part.second->get_part_number() + "</PartNumber>\n"
                    "  <LastModified>" + part.second->get_last_modified_iso() + "</LastModified>\n"
                    "  <ETag>" + part.second->get_md5() + "</ETag>\n"
                    "  <Size>" + part.second->get_content_length_str() + "</Size>\n"
                    "  </Part>\n";
  }

  response_xml += "</ListPartsResult>\n";
  // clang-format on

  return response_xml;
}

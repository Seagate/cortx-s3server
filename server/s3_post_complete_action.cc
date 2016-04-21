/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 6-Jan-2016
 */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <unistd.h>

#include "s3_post_complete_action.h"
#include "s3_error_codes.h"
#include "s3_md5_hash.h"
#include "s3_aws_etag.h"
#include "s3_log.h"

S3PostCompleteAction::S3PostCompleteAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  upload_id = request->get_query_string_value("uploadId");
  object_name = request->get_object_name();
  bucket_name = request->get_bucket_name();
  object_size = 0;
  set_abort_multipart(false);
  setup_steps();
}

void S3PostCompleteAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");

  add_task(std::bind( &S3PostCompleteAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PostCompleteAction::fetch_multipart_info, this ));
  add_task(std::bind( &S3PostCompleteAction::fetch_parts_info, this ));
  add_task(std::bind( &S3PostCompleteAction::save_metadata, this ));
  add_task(std::bind( &S3PostCompleteAction::delete_part_index, this));
  add_task(std::bind( &S3PostCompleteAction::delete_parts, this));
  add_task(std::bind( &S3PostCompleteAction::delete_multipart_metadata, this));
  add_task(std::bind( &S3PostCompleteAction::send_response_to_s3_client, this ));
  // ...
}

void S3PostCompleteAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  std::string input_str = request->get_full_body_content_as_string();
  parse_xml_str(input_str);
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PostCompleteAction::next, this), std::bind( &S3PostCompleteAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::fetch_multipart_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request, true, upload_id);
    object_metadata->load(std::bind( &S3PostCompleteAction::next, this), std::bind( &S3PostCompleteAction::fetch_multipart_info_failed, this));
  } else {
    s3_log(S3_LOG_ERROR, "Missing bucket [%s]\n", request->get_bucket_name().c_str());
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::fetch_multipart_info_failed() {
  s3_log(S3_LOG_ERROR, "Multipart info missing\n");
  send_response_to_s3_client();
}

void S3PostCompleteAction::fetch_parts_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->next_keyval(get_part_index_name(),
                               "",
                               parts.size(),
                               std::bind( &S3PostCompleteAction::get_parts_successful, this),
                               std::bind( &S3PostCompleteAction::get_parts_failed,
                               this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::set_abort_multipart(bool abortit) {
  delete_multipart_object = abortit;
}

bool S3PostCompleteAction::is_abort_multipart() {
  return delete_multipart_object;
}

void S3PostCompleteAction::get_parts_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  S3AwsEtag awsetag;
  size_t curr_size;
  size_t prev_size = 0;

  auto& kvps = clovis_kv_reader->get_key_values();
  part_metadata = std::make_shared<S3PartMetadata>(request, upload_id, 0);
  if(parts.size() != kvps.size()) {
     part_metadata->set_state(S3PartMetadataState::missing_partially);
     send_response_to_s3_client();
  }
  std::map<std::string, std::string>::iterator part_kv;
  std::map<std::string, std::string>::iterator store_kv;

  for (part_kv = parts.begin(), store_kv = kvps.begin(); part_kv != parts.end(); part_kv++, store_kv++) {
    if(part_kv->first != store_kv->first) {
      part_metadata->set_state(S3PartMetadataState::missing);
      send_response_to_s3_client();
    } else {
        s3_log(S3_LOG_DEBUG, "Metadata for key [%s] -> [%s]\n", store_kv->first.c_str(), store_kv->second.c_str());
        part_metadata->from_json(store_kv->second);
        curr_size = part_metadata->get_content_length();
        if(store_kv != kvps.begin()) {
          if(prev_size != curr_size) {
            if(store_kv->first == total_parts) {
              // This is the last part, ignore it after size calculation
              object_size += part_metadata->get_content_length();
              continue;
            }
            s3_log(S3_LOG_ERROR, "The part %s size(%zu) seems to be different from previous part size(%zu), Will be destroying the parts\n",
                   store_kv->first.c_str(),
                   curr_size, prev_size);
            // Will be deleting complete object along with the part index and multipart kv
            set_abort_multipart(true);
            break;
          } else {
             prev_size = curr_size;
            }
        } else {
          prev_size = curr_size;
          }
        object_size += part_metadata->get_content_length();
        awsetag.add_part_etag(part_metadata->get_md5());
      }
   }
  if(!is_abort_multipart()) {
    etag = awsetag.finalize();
  }
  next();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::get_parts_failed() {
  s3_log(S3_LOG_ERROR, "Parts info missing\n");
  send_response_to_s3_client();
}

void S3PostCompleteAction::save_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (is_abort_multipart()) {
    next();
  } else {
    // Mark it as non-multipart, create final object metadata.
    object_metadata->mark_as_non_multipart();
    object_metadata->set_content_length(std::to_string(object_size));
    object_metadata->set_md5(etag);
    object_metadata->save(std::bind( &S3PostCompleteAction::next, this), std::bind( &S3PostCompleteAction::send_response_to_s3_client, this));
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::delete_multipart_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  object_multipart_metadata = std::make_shared<S3ObjectMetadata>(request, true, upload_id);
  object_multipart_metadata->remove(std::bind( &S3PostCompleteAction::delete_multipart_successful, this), std::bind( &S3PostCompleteAction::delete_multipart_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::delete_multipart_successful() {
  s3_log(S3_LOG_DEBUG, "Deleted part metadata\n");
  next();
}

void S3PostCompleteAction::delete_multipart_failed() {
  s3_log(S3_LOG_ERROR, "Delete part metadata failed\n");
  next();
}

void S3PostCompleteAction::delete_part_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata->remove_index(std::bind( &S3PostCompleteAction::delete_part_index_successful, this), std::bind( &S3PostCompleteAction::delete_part_index_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::delete_part_index_successful() {
  s3_log(S3_LOG_DEBUG, "Deleted part index\n");
  next();
}

void S3PostCompleteAction::delete_part_index_failed() {
  s3_log(S3_LOG_ERROR, "Delete index failed for part info\n");
  next();
}

void S3PostCompleteAction::delete_parts() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if(is_abort_multipart()) {
    clovis_writer = std::make_shared<S3ClovisWriter>(request, object_metadata->get_oid());
    clovis_writer->delete_object(std::bind( &S3PostCompleteAction::next, this), std::bind( &S3PostCompleteAction::delete_parts_failed, this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostCompleteAction::delete_parts_failed() {
  s3_log(S3_LOG_DEBUG, "Delete parts info failed.\n");
  send_response_to_s3_client();
}

void S3PostCompleteAction::parse_xml_str(std::string &xml_str) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  xmlNode *child_node;
  xmlChar * xml_part_number;
  xmlChar * xml_etag;
  std::string partnumber;
  std::string prev_partnumber = "";
  int previous_part;
  std::string input_etag;

  s3_log(S3_LOG_DEBUG, "xml string = %s",xml_str.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar*)xml_str.c_str());
  if (document == NULL ) {
    xmlFreeDoc(document);
    s3_log(S3_LOG_ERROR, "The xml string %s is invalid\n", xml_str.c_str());
    send_response_to_s3_client();
  }

  /*Get the root element node */
  xmlNodePtr root_node = xmlDocGetRootElement(document);

  //xmlNodePtr child = root_node->xmlChildrenNode;
  xmlNodePtr child = root_node->xmlChildrenNode;
  while (child != NULL) {
    s3_log(S3_LOG_DEBUG, "Xml Tag = %s\n", (char *)child->name);
    if (!xmlStrcmp(child->name, (const xmlChar *)"Part")) {
      partnumber = "";
      input_etag = "";
      for (child_node = child->children; child_node != NULL; child_node = child_node->next) {
        if ((!xmlStrcmp(child_node->name, (const xmlChar *)"PartNumber"))) {
          xml_part_number = xmlNodeGetContent(child_node);
          if (xml_part_number != NULL) {
            partnumber = (char *)xml_part_number;
            xmlFree(xml_part_number);
            xml_part_number = NULL;
          }
        }
        if ((!xmlStrcmp(child_node->name, (const xmlChar *)"ETag"))) {
          xml_etag = xmlNodeGetContent(child_node);
          if (xml_etag != NULL) {
            input_etag = (char *)xml_etag;
            xmlFree(xml_etag);
            xml_etag = NULL;
          }
        }
      }
      if (!partnumber.empty() && !input_etag.empty()) {
        parts[partnumber] = input_etag;
        if (prev_partnumber.empty()) {
          previous_part = 0;
        } else {
          previous_part = std::stoi(prev_partnumber);
        }
        if (previous_part > std::stoi(partnumber)) {
          // The request doesn't contain part numbers in ascending order
          request->set_request_error(S3RequestError::InvalidPartOrder);
          xmlFreeDoc(document);
          s3_log(S3_LOG_DEBUG, "The XML string doesn't contain parts in ascending order\n");
          send_response_to_s3_client();
        }
        prev_partnumber = partnumber;
      } else {
          s3_log(S3_LOG_DEBUG, "Error: Part number/Etag missing for a part\n");
          xmlFreeDoc(document);
          send_response_to_s3_client();
        }
     }
    child = child->next;
  }
  total_parts = partnumber;
  xmlFreeDoc(document);
  return;
}

void S3PostCompleteAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  S3RequestError req_state = request->get_request_error();
  S3PartMetadataState part_state = S3PartMetadataState::empty;
  if (part_metadata) {
    part_state = part_metadata->get_state();
  }
  if ( bucket_metadata && (bucket_metadata->get_state() == S3BucketMetadataState::missing)) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->send_response(error.get_http_status_code(), response_xml);
    }else if(req_state == S3RequestError::InvalidPartOrder) {
      S3Error error("InvalidPartOrder", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->send_response(error.get_http_status_code(), response_xml);
    }else if(req_state == S3RequestError::EntityTooSmall) {
      S3Error error("EntityTooSmall", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->send_response(error.get_http_status_code(), response_xml);
    }else if(is_abort_multipart()) {
      S3Error error("InvalidObjectState", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->send_response(error.get_http_status_code(), response_xml);
    }else if(part_state == S3PartMetadataState::missing_partially) {
      S3Error error("InvalidPart", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->send_response(error.get_http_status_code(), response_xml);
    } else if(part_state == S3PartMetadataState::missing) {
      S3Error error("InvalidPartOrder", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->send_response(error.get_http_status_code(), response_xml);
    } else if (object_metadata->get_state() == S3ObjectMetadataState::saved) {
      std::string response;
      std::string object_name = request->get_object_name();
      std::string bucket_name = request->get_bucket_name();
      std::string object_uri = request->get_object_uri();
      response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      response += "<CompleteMultipartUploadResult xmlns=\"http://s3\">\n";
      response += "<Location>" + object_uri + "</Location>";
      response += "<Bucket>" + bucket_name + "</Bucket>\n";
      response += "<Key>" + object_name + "</Key>\n";
      response += "<ETag>" + etag + "</ETag>";
      response += "</CompleteMultipartUploadResult>";
      request->send_response(S3HttpSuccess200, response);
    } else {
      S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
      request->send_response(error.get_http_status_code(), response_xml);
    }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

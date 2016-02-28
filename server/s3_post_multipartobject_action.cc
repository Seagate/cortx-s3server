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

#include "s3_post_multipartobject_action.h"
#include "s3_error_codes.h"

S3PostMultipartObjectAction::S3PostMultipartObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3PostMultipartObjectAction::setup_steps(){
  add_task(std::bind( &S3PostMultipartObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::create_upload_id, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::save_metadata, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::create_part_metadata, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PostMultipartObjectAction::fetch_bucket_info() {
  printf("Called S3PostMultipartObjectAction::fetch_bucket_info\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
}

void S3PostMultipartObjectAction::create_upload_id() {
  printf("Called S3PostMultipartObjectAction::create_upload_id\n");
  S3Uuid uuid;

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    upload_id = uuid.get_string_uuid();
    object_metadata = std::make_shared<S3ObjectMetadata>(request, true, upload_id);
    object_metadata->load(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3PostMultipartObjectAction::create_part_metadata() {
  part_metadata = std::make_shared<S3PartMetadata>(request, upload_id, 0);
  part_metadata->create_index(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
}

void S3PostMultipartObjectAction::save_metadata() {
  if (object_metadata->get_state() != S3ObjectMetadataState::present) {
    for (auto it: request->get_in_headers_copy()) {
      if(it.first.find("x-amz-meta-") != std::string::npos) {
        object_metadata->add_user_defined_attribute(it.first, it.second);
      }
    }
    object_metadata->save(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
  } else {
      //++
      // Bailing out in case if the multipart upload is already in progress, this will ensure that object
      // doesn't go to inconsistent state

      // Once multipart is taken care in mero, it may not be needed
      // Note - Currently as per aws doc. (http://docs.aws.amazon.com/AmazonS3/latest/API/mpUploadListMPUpload.html) multipart upload details
      // of the same key initiated at different times is allowed.
      //--
      send_response_to_s3_client();
    }
}

void S3PostMultipartObjectAction::send_response_to_s3_client() {
  printf("Called S3PostMultipartObjectAction::send_response_to_s3_client\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");

    request->send_response(error.get_http_status_code(), response_xml);
  } else if(object_metadata->get_state() == S3ObjectMetadataState::present) {
    S3Error error("InvalidObjectState", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if ((object_metadata->get_state() == S3ObjectMetadataState::saved) &&
    ((part_metadata->get_state() == S3PartMetadataState::store_created) || (part_metadata->get_state() == S3PartMetadataState::present))) {
    std::string response;
    std::string object_name = request->get_object_name();
    std::string bucket_name = request->get_bucket_name();
    response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    response += "<InitiateMultipartUploadResult xmlns=\"http://s3\">\n";
    response += "<Bucket>" + bucket_name + "</Bucket>\n";
    response += "<Key>" + object_name + "</Key>\n";
    response += "<UploadId>" + upload_id + "</UploadId>";
    response += "</InitiateMultipartUploadResult>";
    request->set_out_header_value("UploadId", upload_id);
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
}

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

#include "s3_put_object_action.h"
#include "s3_error_codes.h"

S3PutObjectAction::S3PutObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3PutObjectAction::setup_steps(){
  add_task(std::bind( &S3PutObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PutObjectAction::create_object, this ));
  add_task(std::bind( &S3PutObjectAction::write_object, this ));
  add_task(std::bind( &S3PutObjectAction::save_metadata, this ));
  add_task(std::bind( &S3PutObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutObjectAction::fetch_bucket_info() {
  printf("Called S3PutObjectAction::fetch_bucket_info\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::next, this));
}

void S3PutObjectAction::create_object() {
  printf("Called S3PutObjectAction::create_object\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    clovis_writer = std::make_shared<S3ClovisWriter>(request);
    clovis_writer->create_object(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::create_object_failed, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::create_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3PutObjectAction::create_object_failed\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    // If object exists, S3 overwrites it.
    printf("Existing object: Overwrite it.\n");
    next();
  } else {
    // Any other error report failure.
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::write_object() {
  printf("Called S3PutObjectAction::write_object\n");
  clovis_writer->write_content(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::write_object_failed, this));
}

void S3PutObjectAction::write_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3PutObjectAction::write_object_failed\n");
  send_response_to_s3_client();
}

void S3PutObjectAction::save_metadata() {
  // xxx set attributes & save
  object_metadata = std::make_shared<S3ObjectMetadata>(request);
  object_metadata->set_content_length(request->get_header_value("Content-Length"));
  object_metadata->set_md5(clovis_writer->get_content_md5());
  for (auto it: request->get_in_headers_copy()) {
    if(it.first.find("x-amz-meta-") != std::string::npos) {
      object_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  object_metadata->save(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::next, this));
}

void S3PutObjectAction::send_response_to_s3_client() {
  printf("Called S3PutObjectAction::send_response_to_s3_client\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (clovis_writer->get_state() == S3ClovisWriterOpState::failed) {
    S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::saved) {
    request->set_out_header_value("ETag", clovis_writer->get_content_md5());

    request->send_response(S3HttpSuccess200);
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

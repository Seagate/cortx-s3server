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
#include "s3_clovis_config.h"
#include "s3_error_codes.h"
#include "s3_perf_logger.h"

S3PutObjectAction::S3PutObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), total_data_to_stream(0) {
  setup_steps();
}

void S3PutObjectAction::setup_steps(){
  add_task(std::bind( &S3PutObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PutObjectAction::create_object, this ));
  add_task(std::bind( &S3PutObjectAction::initiate_data_streaming, this ));
  add_task(std::bind( &S3PutObjectAction::save_metadata, this ));
  add_task(std::bind( &S3PutObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutObjectAction::fetch_bucket_info() {
  printf("Called S3PutObjectAction::fetch_bucket_info\n");
  if (!request->get_buffered_input().is_freezed()) {
    request->pause();  // Pause reading till we are ready to consume data.
  }
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::next, this));
}

void S3PutObjectAction::create_object() {
  printf("Called S3PutObjectAction::create_object\n");
  create_object_timer.start();
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    clovis_writer = std::make_shared<S3ClovisWriter>(request);
    clovis_writer->create_object(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::create_object_failed, this));
  } else {
    request->resume();
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
    create_object_timer.stop();
    LOG_PERF("create_object_failed_ms", create_object_timer.elapsed_time_in_millisec());

    request->resume();
    // Any other error report failure.
    send_response_to_s3_client();
  }
}

void S3PutObjectAction::initiate_data_streaming() {
  printf("Called S3PutObjectAction::initiate_data_streaming\n");
  create_object_timer.stop();
  LOG_PERF("create_object_successful_ms", create_object_timer.elapsed_time_in_millisec());

  total_data_to_stream = request->get_content_length();
  request->resume();

  if (total_data_to_stream == 0) {
    save_metadata();  // Zero size object.
  } else {
    if (request->has_all_body_content()) {
      printf("We have all the data, so just write it....\n");
      write_object(request->get_buffered_input());
    } else {
      printf("We do not have all the data, so start listening....\n");
      // Start streaming, logically pausing action till we get data.
      request->listen_for_incoming_data(
          std::bind(&S3PutObjectAction::consume_incoming_content, this),
          S3ClovisConfig::get_instance()->get_clovis_write_payload_size()
        );
    }
  }
}

void S3PutObjectAction::consume_incoming_content() {
  printf("Called S3PutObjectAction::consume_incoming_content\n");
  // Resuming the action since we have data.
  write_object(request->get_buffered_input());
}

void S3PutObjectAction::write_object(S3AsyncBufferContainer& buffer) {
  printf("Called S3PutObjectAction::write_object\n");
  if (buffer.is_freezed()) {
    // This is last one, no more data ahead.
    printf("This is last one, no more data ahead, so S3ClovisWriter write_content.\n");
    clovis_writer->write_content(std::bind( &S3PutObjectAction::write_object_successful, this), std::bind( &S3PutObjectAction::write_object_failed, this), buffer);
  } else {
    request->pause();  // Pause till write to clovis is complete
    printf("We will still be expecting more data, so S3ClovisWriter write_content and pause to wait for more data\n");
    // We will still be expecting more data, so write and pause to wait for more data
    clovis_writer->write_content(std::bind( &S3RequestObject::resume, request), std::bind( &S3PutObjectAction::write_object_failed, this), buffer);
  }
}

void S3PutObjectAction::write_object_successful() {
  printf("Called S3PutObjectAction::write_object_successful\n");
  if (request->get_buffered_input().length() > 0) {
    // We still have more data to write.
    write_object(request->get_buffered_input());
  } else {
    next();
  }
}

void S3PutObjectAction::write_object_failed() {
  printf("Called S3PutObjectAction::write_object_failed\n");
  send_response_to_s3_client();
}

void S3PutObjectAction::save_metadata() {
  // xxx set attributes & save
  object_metadata = std::make_shared<S3ObjectMetadata>(request);
  object_metadata->set_content_length(request->get_header_value("Content-Length"));
  object_metadata->set_md5(clovis_writer->get_content_md5());
  object_metadata->set_oid(clovis_writer->get_oid());
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
  request->resume();

  done();
  i_am_done();  // self delete
}

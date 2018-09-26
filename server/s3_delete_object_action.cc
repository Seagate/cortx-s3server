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

#include "s3_delete_object_action.h"
#include "s3_error_codes.h"
#include "s3_iem.h"

S3DeleteObjectAction::S3DeleteObjectAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<S3ClovisWriterFactory> writer_factory)
    : S3Action(req, false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id,
         "S3 API: Delete Object API. Bucket[%s] Object[%s]\n",
         request->get_bucket_name().c_str(),
         request->get_object_name().c_str());

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  if (object_meta_factory) {
    object_metadata_factory = object_meta_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }

  if (writer_factory) {
    clovis_writer_factory = writer_factory;
  } else {
    clovis_writer_factory = std::make_shared<S3ClovisWriterFactory>();
  }

  setup_steps();
}

void S3DeleteObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setup the action\n");
  add_task(std::bind(&S3DeleteObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3DeleteObjectAction::fetch_object_info, this));
  // We delete metadata first followed by object, since if we delete
  // the object first and say for some reason metadata clean up fails,
  // If this "oid" gets allocated to some other object, current object
  // metadata will point to someone else's object leading 2 problems:
  // accessing someone else's object and second retrying delete, deleting
  // someone else's object. Whereas deleting metadata first will worst case
  // lead to object leak in mero which can handle separately.
  // To delete stale objects: ref: MINT-602
  add_task(std::bind(&S3DeleteObjectAction::delete_metadata, this));
  add_task(std::bind(&S3DeleteObjectAction::delete_object, this));
  add_task(std::bind(&S3DeleteObjectAction::send_response_to_s3_client, this));
  // ...
}

void S3DeleteObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(std::bind(&S3DeleteObjectAction::next, this),
                        std::bind(&S3DeleteObjectAction::next, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::fetch_object_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    struct m0_uint128 object_list_indx_oid =
        bucket_metadata->get_object_list_index_oid();
    if (object_list_indx_oid.u_hi == 0ULL &&
        object_list_indx_oid.u_lo == 0ULL) {
      // There is no object list index, hence object doesn't exist
      s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
      send_response_to_s3_client();
    } else {
      object_metadata = object_metadata_factory->create_object_metadata_obj(
          request, object_list_indx_oid);

      object_metadata->load(std::bind(&S3DeleteObjectAction::next, this),
                            std::bind(&S3DeleteObjectAction::next, this));
    }
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_WARN, request_id, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_WARN, request_id, "Failed to look up Bucket metadata\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::delete_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    object_metadata->remove(std::bind(&S3DeleteObjectAction::next, this),
                            std::bind(&S3DeleteObjectAction::next, this));
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_WARN, request_id, "Object not found\n");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_WARN, request_id, "Failed to look up Object metadata\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3DeleteObjectAction::delete_object() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (object_metadata->get_state() == S3ObjectMetadataState::deleted) {
    clovis_writer = clovis_writer_factory->create_clovis_writer(
        request, object_metadata->get_oid());
    clovis_writer->delete_object(
        std::bind(&S3DeleteObjectAction::next, this),
        std::bind(&S3DeleteObjectAction::delete_object_failed, this),
        object_metadata->get_layout_id());
  } else {
    s3_log(S3_LOG_WARN, request_id, "Failed to delete Object metadata\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047006001</event_code>
 *    <application>S3 Server</application>
 *    <submodule>S3 Actions</submodule>
 *    <description>Delete object failed causing stale data in Mero</description>
 *    <audience>Development</audience>
 *    <details>
 *      Delete object op failed. It may cause stale data in Mero.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files.
 *      Contact development team for further investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

void S3DeleteObjectAction::delete_object_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    next();
  } else if (clovis_writer->get_state() ==
             S3ClovisWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Deletion of object with oid "
           "%" SCNx64 " : %" SCNx64 " failed\n",
           object_metadata->get_oid().u_hi, object_metadata->get_oid().u_lo);
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    // Any other error report failure.
    s3_log(S3_LOG_ERROR, request_id,
           "Deletion of object with oid "
           "%" SCNx64 " : %" SCNx64 " failed\n",
           object_metadata->get_oid().u_hi, object_metadata->get_oid().u_lo);
    s3_iem(LOG_ERR, S3_IEM_DELETE_OBJ_FAIL, S3_IEM_DELETE_OBJ_FAIL_STR,
           S3_IEM_DELETE_OBJ_FAIL_JSON);
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}


void S3DeleteObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (is_error_state() && !get_s3_error_code().empty()) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }

    request->send_response(error.get_http_status_code(), response_xml);
  } else {
    request->send_response(S3HttpSuccess204);
  }

  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}

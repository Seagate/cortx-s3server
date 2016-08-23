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

#include "s3_api_handler.h"
#include "s3_head_object_action.h"
#include "s3_get_object_action.h"
#include "s3_put_object_action.h"
#include "s3_put_chunk_upload_object_action.h"
#include "s3_put_multiobject_action.h"
#include "s3_post_multipartobject_action.h"
#include "s3_post_complete_action.h"
#include "s3_delete_object_action.h"
#include "s3_get_multipart_part_action.h"
#include "s3_abort_multipart_action.h"
#include "s3_get_object_acl_action.h"
#include "s3_put_object_acl_action.h"
#include "s3_log.h"

void S3ObjectAPIHandler::dispatch() {
  std::shared_ptr<S3Action> action;

  s3_log(S3_LOG_DEBUG, "Operation code = %d\n", operation_code);

  switch(operation_code) {
    case S3OperationCode::acl:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetObjectACLAction>(request);
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutObjectACLAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    case S3OperationCode::multipart:
      switch (request->http_verb()) {
        case S3HttpVerb::POST:
           if (request->has_query_param_key("uploadid")) {
             // Complete multipart upload
             action = std::make_shared<S3PostCompleteAction>(request);
           } else {
            // Initiate Multipart
            action = std::make_shared<S3PostMultipartObjectAction>(request);
           }
          break;
        case S3HttpVerb::PUT:
          if (!request->get_header_value("x-amz-copy-source").empty()) {
            // Copy Object in part upload not yet supported.
            // Do nothing = unsupported API
          } else {
            // Multipart part uploads
            action = std::make_shared<S3PutMultiObjectAction>(request);
          }
          break;
        case S3HttpVerb::GET:
          // Multipart part listing
          action = std::make_shared<S3GetMultipartPartAction>(request);
          break;
        case S3HttpVerb::DELETE:
          // Multipart abort
          action = std::make_shared<S3AbortMultipartAction>(request);
          break;
      };
      break;
    case S3OperationCode::none:
      // Perform operation on Object.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          action = std::make_shared<S3HeadObjectAction>(request);
          break;
        case S3HttpVerb::PUT:
          if (request->get_header_value("x-amz-content-sha256") == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD") {
            // chunk upload
            action = std::make_shared<S3PutChunkUploadObjectAction>(request);
          } else if (!request->get_header_value("x-amz-copy-source").empty()) {
            // Copy Object not yet supported.
            // Do nothing = unsupported API
          } else {
            // single chunk upload
            action = std::make_shared<S3PutObjectAction>(request);
          }
          break;
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetObjectAction>(request);
          break;
        case S3HttpVerb::DELETE:
          action = std::make_shared<S3DeleteObjectAction>(request);
          break;
        case S3HttpVerb::POST:
          // action = std::make_shared<S3PostObjectAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    default:
      // should never be here.
      request->respond_unsupported_api();
      i_am_done();
      return;
  };  // switch operation_code
  if (action) {
    action->manage_self(action);
    action->start();
  } else {
    request->respond_unsupported_api();
  }
  i_am_done();
}

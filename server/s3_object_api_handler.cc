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
#include "s3_delete_object_action.h"

void S3ObjectAPIHandler::dispatch() {
  std::shared_ptr<S3Action> action;
  switch(operation_code) {
    case S3OperationCode::acl:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          // action = std::make_shared<S3GetObjectACLAction>(request);
          break;
        case S3HttpVerb::PUT:
          // action = std::make_shared<S3PutObjectACLAction>(request);
          break;
        default:
          // should never be here.
          request->respond_unsupported_api();
          i_am_done();
          return;
      };
      break;
    case S3OperationCode::none:
      // Perform operation on Object.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          action = std::make_shared<S3HeadObjectAction>(request);
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutObjectAction>(request);
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

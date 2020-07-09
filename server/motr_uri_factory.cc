/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-June-2010
 */

#include "motr_uri.h"
#include "s3_log.h"

MotrURI* MotrUriFactory::create_uri_object(
    MotrUriType uri_type, std::shared_ptr<MotrRequestObject> request) {
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "Entering\n");

  switch (uri_type) {
    case MotrUriType::path_style:
      s3_log(S3_LOG_DEBUG, request->get_request_id(),
             "Creating motr path_style object\n");
      return new MotrPathStyleURI(request);
    default:
      break;
  };
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "Exiting\n");
  return NULL;
}

/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include "s3_uri.h"
#include "s3_log.h"

S3URI* S3UriFactory::create_uri_object(
    S3UriType uri_type, std::shared_ptr<S3RequestObject> request) {
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "%s Entry\n", __func__);

  switch (uri_type) {
    case S3UriType::path_style:
      s3_log(S3_LOG_DEBUG, request->get_request_id(),
             "Creating path_style object\n");
      return new S3PathStyleURI(request);
    case S3UriType::virtual_host_style:
      s3_log(S3_LOG_DEBUG, request->get_request_id(),
             "Creating virtual_host_style object\n");
      return new S3VirtualHostStyleURI(request);
    default:
      break;
  };
  s3_log(S3_LOG_DEBUG, request->get_request_id(), "%s Exit", __func__);
  return NULL;
}

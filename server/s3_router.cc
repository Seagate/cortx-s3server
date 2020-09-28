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

#include <memory>
#include <regex>
#include <string>

#include "s3_api_handler.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_router.h"
#include "s3_stats.h"
#include "s3_uri.h"
#include "motr_request_object.h"

bool Router::is_default_endpoint(std::string& endpoint) {
  return S3Option::get_instance()->get_default_endpoint() == endpoint;
}

bool Router::is_exact_valid_endpoint(std::string& endpoint) {
  if (endpoint == S3Option::get_instance()->get_default_endpoint()) {
    return true;
  }
  return S3Option::get_instance()->get_region_endpoints().find(endpoint) !=
         S3Option::get_instance()->get_region_endpoints().end();
}

bool Router::is_subdomain_match(std::string& endpoint) {
  // todo check if given endpoint is subdomain or default or region.
  if (endpoint.find(S3Option::get_instance()->get_default_endpoint()) !=
      std::string::npos) {
    return true;
  }
  for (std::set<std::string>::iterator it =
           S3Option::get_instance()->get_region_endpoints().begin();
       it != S3Option::get_instance()->get_region_endpoints().end(); ++it) {
    if (endpoint.find(*it) != std::string::npos) {
      return true;
    }
  }
  return false;
}

S3Router::S3Router(S3APIHandlerFactory* api_creator, S3UriFactory* uri_creator)
    : api_handler_factory(api_creator), uri_factory(uri_creator) {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
}

S3Router::~S3Router() {
  s3_log(S3_LOG_DEBUG, "", "Destructor\n");
  delete api_handler_factory;
  delete uri_factory;
}

void S3Router::dispatch(std::shared_ptr<RequestObject> request) {
  std::string request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  std::shared_ptr<S3RequestObject> s3request =
      std::dynamic_pointer_cast<S3RequestObject>(request);
  std::shared_ptr<S3APIHandler> handler;

  std::string host_name = request->get_host_name();

  s3_log(S3_LOG_DEBUG, request_id, "host_name = %s\n", host_name.c_str());
  s3_log(S3_LOG_DEBUG, request_id, "uri = %s\n", s3request->c_get_full_path());

  std::unique_ptr<S3URI> uri;
  S3UriType uri_type = S3UriType::unsupported;

  if (host_name.empty() || is_exact_valid_endpoint(host_name) ||
      s3request->is_valid_ipaddress(host_name) ||
      !is_subdomain_match(host_name)) {
    // Path style API
    // Bucket for the request will be the first slash-delimited component of the
    // Request-URI
    s3_log(S3_LOG_DEBUG, request_id, "Detected S3PathStyleURI\n");
    uri_type = S3UriType::path_style;
  } else {
    // Virtual host style endpoint
    s3_log(S3_LOG_DEBUG, request_id, "Detected S3VirtualHostStyleURI\n");
    uri_type = S3UriType::virtual_host_style;
  }

  uri = std::unique_ptr<S3URI>(
      uri_factory->create_uri_object(uri_type, s3request));

  s3request->set_bucket_name(uri->get_bucket_name());
  s3request->set_object_name(uri->get_object_name());
  s3_log(S3_LOG_DEBUG, request_id, "Detected bucket name = %s\n",
         uri->get_bucket_name().c_str());
  s3_log(S3_LOG_DEBUG, request_id, "Detected object name = %s\n",
         uri->get_object_name().c_str());

  handler = api_handler_factory->create_api_handler(
      uri->get_s3_api_type(), s3request, uri->get_operation_code());

  if (handler) {
    s3_stats_inc("total_request_count");
    handler->manage_self(handler);
    handler->dispatch();  // Start processing the request
  } else {
    s3request->respond_unsupported_api();
  }
  return;
}

MotrRouter::MotrRouter(MotrAPIHandlerFactory* api_creator,
                       MotrUriFactory* uri_creator)
    : api_handler_factory(api_creator), uri_factory(uri_creator) {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
}

MotrRouter::~MotrRouter() {
  s3_log(S3_LOG_DEBUG, "", "Destructor\n");
  delete api_handler_factory;
  delete uri_factory;
}

void MotrRouter::dispatch(std::shared_ptr<RequestObject> request) {
  std::string request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  std::shared_ptr<MotrRequestObject> motr_request =
      std::dynamic_pointer_cast<MotrRequestObject>(request);
  std::shared_ptr<MotrAPIHandler> handler;

  std::string host_name = motr_request->get_host_name();

  s3_log(S3_LOG_DEBUG, request_id, "host_name = %s\n", host_name.c_str());
  s3_log(S3_LOG_INFO, request_id, "uri = %s\n",
         motr_request->c_get_full_path());

  std::unique_ptr<MotrURI> uri;
  MotrUriType uri_type = MotrUriType::unsupported;

  if (host_name.empty() || is_exact_valid_endpoint(host_name) ||
      motr_request->is_valid_ipaddress(host_name) ||
      !is_subdomain_match(host_name)) {
    // Path style API
    // Request-URI
    s3_log(S3_LOG_DEBUG, request_id, "Detected Motr PathStyleURI\n");
    uri_type = MotrUriType::path_style;
  }

  uri = std::unique_ptr<MotrURI>(
      uri_factory->create_uri_object(uri_type, motr_request));

  motr_request->set_key_name(uri->get_key_name());
  motr_request->set_object_oid_lo(uri->get_object_oid_lo());
  motr_request->set_object_oid_hi(uri->get_object_oid_hi());
  motr_request->set_index_id_lo(uri->get_index_id_lo());
  motr_request->set_index_id_hi(uri->get_index_id_hi());

  handler = api_handler_factory->create_api_handler(
      uri->get_motr_api_type(), motr_request, uri->get_operation_code());

  if (handler) {
    s3_stats_inc("total_motr_request_count");
    handler->manage_self(handler);
    handler->dispatch();  // Start processing the request
  } else {
    motr_request->respond_unsupported_api();
  }
  return;
}

// S3 URL Patterns for various APIs.

// List Buckets  -> http://s3.seagate.com/ GET

// Bucket APIs   -> http://s3.seagate.com/bucketname
// Host Header = s3.seagate.com or empty
//               -> http://bucketname.s3.seagate.com
// Host Header = *.s3.seagate.com
// Host Header = anything else = bucketname
// location      -> http://s3.seagate.com/bucketname?location
// ACL           -> http://s3.seagate.com/bucketname?acl
//               -> http://bucketname.s3.seagate.com/?acl
// Policy        -> http://s3.seagate.com/bucketname?policy
// Logging       -> http://s3.seagate.com/bucketname?logging
// Lifecycle     -> http://s3.seagate.com/bucketname?lifecycle
// CORs          -> http://s3.seagate.com/bucketname?cors
// notification  -> http://s3.seagate.com/bucketname?notification
// replicaton    -> http://s3.seagate.com/bucketname?replicaton
// tagging       -> http://s3.seagate.com/bucketname?tagging
// requestPayment-> http://s3.seagate.com/bucketname?requestPayment
// versioning    -> http://s3.seagate.com/bucketname?versioning
// website       -> http://s3.seagate.com/bucketname?website
// list multipart uploads in progress
//               -> http://s3.seagate.com/bucketname?uploads

// Object APIs   -> http://s3.seagate.com/bucketname/ObjectKey
// Host Header = s3.seagate.com or empty
//               -> http://bucketname.s3.seagate.com/ObjectKey
// Host Header = *.s3.seagate.com
// Host Header = anything else = bucketname

// ACL           -> http://s3.seagate.com/bucketname/ObjectKey?acl
//               -> http://bucketname.s3.seagate.com/ObjectKey?acl

// Multiple dels -> http://s3.seagate.com/bucketname?delete

// Initiate multipart upload
//               -> http://s3.seagate.com/bucketname/ObjectKey?uploads
// Upload part   ->
// http://s3.seagate.com/bucketname/ObjectKey?partNumber=PartNumber&uploadId=UploadId
// Complete      -> http://s3.seagate.com/bucketname/ObjectKey?uploadId=UploadId
// Abort         -> http://s3.seagate.com/bucketname/ObjectKey?uploadId=UploadId
// DEL


#include <memory>

#include "s3_router.h"

void S3Router::setup() {
  default_endpoint = "s3.seagate.com";  // Default region endpoint (US)
  // TODO - load region endpoints from config
  std::string eps[] = {"s3-us.seagate.com", "s3-europe.seagate.com", "s3-asia.seagate.com"};
  for( int i = 0; i < 3; i++) {
    region_endpoints.insert(eps[i]);
  }
}

bool S3Router::is_default_endpoint(std::string& endpoint) {
  return default_endpoint == endpoint;
}

bool S3Router::is_exact_valid_endpoint(std::string& endpoint) {
  return region_endpoints.find(endpoint) != region_endpoints.end();
}

bool S3Router::is_subdomain_match(std::string& endpoint) {
  // todo check if given endpoint is subdomain or default or region.
  return true;
}

void S3Router::dispatch(evhtp_request_t * req) {
  S3RequestObject request(req);

  std::string bucket_name, object_name;

  std::string host_header = request.get_host_header();
  std::unique_ptr<S3URI> uri;
  if ( host_header.empty() || is_exact_valid_endpoint(host_header)) {
    // Path style API
    // Bucket for the request will be the first slash-delimited component of the Request-URI
    uri = std::unique_ptr<S3URI>(new S3PathStyleURI(request.c_get_full_path()));
  } else {
    // Virtual host style endpoint
    uri = std::unique_ptr<S3URI>(new S3VirtualHostStyleURI(host_header, request.c_get_full_path()));
  }
  request.set_bucket_name(uri->bucket_name());
  request.set_object_name(uri->object_name());

  std::shared_ptr<S3APIHandler> handler;
  if (uri->is_service_api()) {
    handler = std::make_shared<S3ServiceAPIHandler> (request, uri->operation_code());
  } else if (uri->is_bucket_api()) {
    handler = std::make_shared<S3BucketAPIHandler> (request, uri->operation_code());
  } else if (uri->is_object_api()) {
    handler = std::make_shared<S3ObjectAPIHandler>(request, uri->operation_code());
  } else {
    // Unsupported
    request.respond_unsupported_api();
    return;
  }
  handler->manage_self(handler);
  handler->dispatch();  // Start processing the request.
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
// Upload part   -> http://s3.seagate.com/bucketname/ObjectKey?partNumber=PartNumber&uploadId=UploadId
// Complete      -> http://s3.seagate.com/bucketname/ObjectKey?uploadId=UploadId
// Abort         -> http://s3.seagate.com/bucketname/ObjectKey?uploadId=UploadId DEL


#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_ROUTER_H__
#define __MERO_FE_S3_SERVER_S3_ROUTER_H__

#include <string>
/* libevhtp */
#include <evhtp.h>

// Not thread-safe, but we are single threaded.
class S3Router {

private:
  static S3Router* instance;
  S3Router();

private:
  // Some way to map URL pattern with handler.
  bool is_default_endpoint(std::string& endpoint);
  bool is_exact_valid_endpoint(std::string& endpoint);
  bool is_subdomain_match(std::string& endpoint);

public:
  static S3Router* get_instance();

  // Dispatch to registered handlers.
  void dispatch(evhtp_request_t * req);
};

#endif

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

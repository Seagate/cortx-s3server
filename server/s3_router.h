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

#pragma once

#ifndef __S3_SERVER_S3_ROUTER_H__
#define __S3_SERVER_S3_ROUTER_H__

#include <string>
/* libevhtp */
#include <evhtp.h>

#include <gtest/gtest_prod.h>

#include "s3_api_handler.h"
#include "motr_api_handler.h"
#include "s3_uri.h"
#include "motr_uri.h"

class Router {
 protected:
  // Some way to map URL pattern with handler.
  bool is_default_endpoint(std::string &endpoint);
  bool is_exact_valid_endpoint(std::string &endpoint);
  bool is_subdomain_match(std::string &endpoint);

 public:
  Router() = default;
  virtual ~Router() {};
  // Dispatch to registered handlers.
  virtual void dispatch(std::shared_ptr<RequestObject> request) = 0;
};

// Not thread-safe, but we are single threaded.
class S3Router : public Router {
 private:
  S3APIHandlerFactory *api_handler_factory;
  S3UriFactory *uri_factory;

 public:
  S3Router(S3APIHandlerFactory *api_creator, S3UriFactory *uri_creator);
  virtual ~S3Router();
  // Dispatch to registered handlers.
  void dispatch(std::shared_ptr<RequestObject> request);

  // For Unit testing only.
  FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingDefaultEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForMisMatchOfDefaultEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForEmptyDefaultEP);
  FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingEP);
  FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingRegionEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForMisMatchRegionEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForEmptyRegionEP);
  FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingSubEP);
  FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingSubRegionEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForMisMatchSubRegionEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForInvalidEP);
  FRIEND_TEST(S3RouterTest, ReturnsFalseForEmptyEP);
  FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingEUSubRegionEP);
};

class MotrRouter : public Router {
 private:
  MotrAPIHandlerFactory *api_handler_factory;
  MotrUriFactory *uri_factory;

 public:
  MotrRouter(MotrAPIHandlerFactory *api_creator, MotrUriFactory *uri_creator);
  virtual ~MotrRouter();
  // Dispatch to registered handlers.
  void dispatch(std::shared_ptr<RequestObject> request);

  // For Unit testing only.
  // FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingDefaultEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForMisMatchOfDefaultEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForEmptyDefaultEP);
  // FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingEP);
  // FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingRegionEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForMisMatchRegionEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForEmptyRegionEP);
  // FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingSubEP);
  // FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingSubRegionEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForMisMatchSubRegionEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForInvalidEP);
  // FRIEND_TEST(S3RouterTest, ReturnsFalseForEmptyEP);
  // FRIEND_TEST(S3RouterTest, ReturnsTrueForMatchingEUSubRegionEP);
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
// Upload part   ->
// http://s3.seagate.com/bucketname/ObjectKey?partNumber=PartNumber&uploadId=UploadId
// Complete      -> http://s3.seagate.com/bucketname/ObjectKey?uploadId=UploadId
// Abort         -> http://s3.seagate.com/bucketname/ObjectKey?uploadId=UploadId
// DEL

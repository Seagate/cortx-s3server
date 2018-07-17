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
#include "s3_delete_bucket_action.h"
#include "s3_delete_bucket_policy_action.h"
#include "s3_delete_multiple_objects_action.h"
#include "s3_get_bucket_acl_action.h"
#include "s3_get_bucket_action.h"
#include "s3_get_bucket_location_action.h"
#include "s3_get_bucket_policy_action.h"
#include "s3_get_multipart_bucket_action.h"
#include "s3_head_bucket_action.h"
#include "s3_log.h"
#include "s3_put_bucket_acl_action.h"
#include "s3_put_bucket_action.h"
#include "s3_put_bucket_policy_action.h"
#include "s3_stats.h"

void S3BucketAPIHandler::create_action() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_INFO, request_id, "Action operation code = %d\n",
         operation_code);

  switch (operation_code) {
    case S3OperationCode::location:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetBucketlocationAction>(request);
          s3_stats_inc("get_bucket_location_request_count");
          break;
        case S3HttpVerb::PUT:
          // action = std::make_shared<S3PutBucketlocationAction>(request);
          break;
        default:
          // should never be here.
          return;
      };
      break;
    case S3OperationCode::multidelete:
      action = std::make_shared<S3DeleteMultipleObjectsAction>(request);
      s3_stats_inc("delete_multiobject_request_count");
      break;
    case S3OperationCode::acl:
      // ACL operations.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetBucketACLAction>(request);
          s3_stats_inc("get_bucket_acl_request_count");
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutBucketACLAction>(request);
          s3_stats_inc("put_bucket_acl_request_count");
          break;
        default:
          // should never be here.
          return;
      };
      break;
    case S3OperationCode::multipart:
      // Perform multipart operation on Bucket.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetMultipartBucketAction>(request);
          s3_stats_inc("get_multipart_bucket_request_count");
          break;
        default:
          // should never be here.
          return;
      };
      break;
    case S3OperationCode::policy:
      // Policy operations.
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          action = std::make_shared<S3GetBucketPolicyAction>(request);
          s3_stats_inc("get_bucket_policy_request_count");
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutBucketPolicyAction>(request);
          s3_stats_inc("put_bucket_policy_request_count");
          break;
        case S3HttpVerb::DELETE:
          action = std::make_shared<S3DeleteBucketPolicyAction>(request);
          s3_stats_inc("delete_bucket_policy_request_count");
          break;
        default:
          // should never be here.
          return;
      };
      break;

    case S3OperationCode::analytics:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_analytics_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_analytics_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_analytics_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::inventory:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_inventory_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_inventory_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_inventory_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::metrics:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_metrics_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_metrics_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_metrics_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::tagging:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_tagging_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_tagging_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_tagging_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::website:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_website_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_website_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_website_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::replication:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_replication_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_replication_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_replication_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::accelerate:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_accelerate_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_accelerate_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::logging:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_logging_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_logging_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::versioning:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_versioning_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_versioning_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::notification:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_notification_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_notification_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::requestPayment:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_requestPayment_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_requestPayment_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::encryption:
      switch (request->http_verb()) {
        case S3HttpVerb::GET:
          s3_stats_inc("get_bucket_encryption_count");
          break;
        case S3HttpVerb::PUT:
          s3_stats_inc("put_bucket_encryption_count");
          break;
        case S3HttpVerb::DELETE:
          s3_stats_inc("delete_bucket_encryption_count");
          break;
        default:
          return;
      }
      break;
    case S3OperationCode::none:
      // Perform operation on Bucket.
      switch (request->http_verb()) {
        case S3HttpVerb::HEAD:
          action = std::make_shared<S3HeadBucketAction>(request);
          s3_stats_inc("head_bucket_request_count");
          break;
        case S3HttpVerb::GET:
          // List Objects in bucket
          action = std::make_shared<S3GetBucketAction>(request);
          s3_stats_inc("get_bucket_request_count");
          break;
        case S3HttpVerb::PUT:
          action = std::make_shared<S3PutBucketAction>(request);
          s3_stats_inc("put_bucket_request_count");
          break;
        case S3HttpVerb::DELETE:
          action = std::make_shared<S3DeleteBucketAction>(request);
          s3_stats_inc("delete_bucket_request_count");
          break;
        case S3HttpVerb::POST:
          // action = std::make_shared<S3PostBucketAction>(request);
          break;
        default:
          // should never be here.
          return;
      };
      break;
    default:
      // should never be here.
      return;
  };  // switch operation_code
  s3_log(S3_LOG_DEBUG, request_id, "Exiting");
}

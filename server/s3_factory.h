/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar        <rajesh.nambiar"seagate.com>
 * Original creation date: 20-Jan-2017
 */

#pragma once

#ifndef __S3_SERVER_S3_FACTORY_H__
#define __S3_SERVER_S3_FACTORY_H__

#include "s3_bucket_metadata.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"

class S3BucketMetadataFactory {
 public:
  virtual ~S3BucketMetadataFactory() {}
  virtual std::shared_ptr<S3BucketMetadata> create_bucket_metadata_obj(
      std::shared_ptr<S3RequestObject> req) {
    s3_log(S3_LOG_DEBUG,
           "S3BucketMetadataFactory::create_bucket_metadata_obj\n");
    return std::make_shared<S3BucketMetadata>(req);
  }
};

class S3ObjectMetadataFactory {
 public:
  virtual ~S3ObjectMetadataFactory() {}
  virtual std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req, m0_uint128 indx_oid) {
    s3_log(S3_LOG_DEBUG,
           "S3ObjectMetadataFactory::create_object_metadata_obj\n");
    return std::make_shared<S3ObjectMetadata>(req, indx_oid);
  }
};

class S3ObjectMultipartMetadataFactory {
 public:
  virtual ~S3ObjectMultipartMetadataFactory() {}
  virtual std::shared_ptr<S3ObjectMetadata> create_object_mp_metadata_obj(
      std::shared_ptr<S3RequestObject> req, m0_uint128 mp_indx_oid,
      bool is_multipart, std::string upload_id) {
    s3_log(S3_LOG_DEBUG,
           "S3ObjectMetadataFactory::create_object_mp_metadata_obj\n");
    return std::make_shared<S3ObjectMetadata>(req, mp_indx_oid, is_multipart,
                                              upload_id);
  }
};

class S3PartMetadataFactory {
 public:
  virtual ~S3PartMetadataFactory() {}
  virtual std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, m0_uint128 indx_oid,
      std::string upload_id, int part_num) {
    s3_log(S3_LOG_DEBUG, "S3ObjectMetadataFactory::create_part_metadata_obj\n");
    return std::make_shared<S3PartMetadata>(req, indx_oid, upload_id, part_num);
  }
  virtual std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, std::string upload_id,
      int part_num) {
    s3_log(S3_LOG_DEBUG, "S3ObjectMetadataFactory::create_part_metadata_obj\n");
    return std::make_shared<S3PartMetadata>(req, upload_id, part_num);
  }
};

class S3ClovisWriterFactory {
 public:
  virtual ~S3ClovisWriterFactory() {}
  virtual std::shared_ptr<S3ClovisWriter> create_clovis_writer(
      std::shared_ptr<S3RequestObject> req, m0_uint128 oid) {
    s3_log(S3_LOG_DEBUG, "S3ClovisWriterFactory::create_clovis_writer\n");
    return std::make_shared<S3ClovisWriter>(req, oid);
  }
};

#endif

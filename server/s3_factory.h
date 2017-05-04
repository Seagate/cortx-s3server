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

#include "s3_async_buffer_opt.h"
#include "s3_auth_client.h"
#include "s3_bucket_metadata.h"
#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_kvs_writer.h"
#include "s3_clovis_reader.h"
#include "s3_clovis_writer.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"
#include "s3_put_bucket_body.h"

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
      std::shared_ptr<S3RequestObject> req,
      m0_uint128 indx_oid = {0ULL, 0ULL}) {
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
           "S3ObjectMultipartMetadataFactory::create_object_mp_metadata_obj\n");
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
    s3_log(S3_LOG_DEBUG, "S3PartMetadataFactory::create_part_metadata_obj\n");
    return std::make_shared<S3PartMetadata>(req, indx_oid, upload_id, part_num);
  }
  virtual std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, std::string upload_id,
      int part_num) {
    s3_log(S3_LOG_DEBUG, "S3PartMetadataFactory::create_part_metadata_obj\n");
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
  virtual std::shared_ptr<S3ClovisWriter> create_clovis_writer(
      std::shared_ptr<S3RequestObject> req) {
    s3_log(S3_LOG_DEBUG,
           "S3ClovisWriterFactory::create_clovis_writer with zero offset\n");
    return std::make_shared<S3ClovisWriter>(req);
  }
  virtual std::shared_ptr<S3ClovisWriter> create_clovis_writer(
      std::shared_ptr<S3RequestObject> req, m0_uint128 oid, uint64_t offset) {
    s3_log(S3_LOG_DEBUG,
           "S3ClovisWriterFactory::create_clovis_writer with offset\n");
    return std::make_shared<S3ClovisWriter>(req, oid, offset);
  }
};

class S3ClovisReaderFactory {
 public:
  virtual ~S3ClovisReaderFactory() {}

  virtual std::shared_ptr<S3ClovisReader> create_clovis_reader(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr) {
    s3_log(S3_LOG_DEBUG, "S3ClovisReaderFactory::create_clovis_reader\n");
    return std::make_shared<S3ClovisReader>(req, clovis_api);
  }

  virtual std::shared_ptr<S3ClovisReader> create_clovis_reader(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 id,
      std::shared_ptr<ClovisAPI> clovis_api = nullptr) {
    s3_log(S3_LOG_DEBUG, "S3ClovisReaderFactory::create_clovis_reader\n");
    return std::make_shared<S3ClovisReader>(req, id, clovis_api);
  }
};

class S3ClovisKVSReaderFactory {
 public:
  virtual ~S3ClovisKVSReaderFactory() {}
  virtual std::shared_ptr<S3ClovisKVSReader> create_clovis_kvs_reader(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> s3_clovis_api = nullptr) {
    s3_log(S3_LOG_DEBUG,
           "S3ClovisKVSReaderFactory::create_clovis_kvs_reader\n");
    return std::make_shared<S3ClovisKVSReader>(req, s3_clovis_api);
  }
};

class S3ClovisKVSWriterFactory {
 public:
  virtual ~S3ClovisKVSWriterFactory() {}
  virtual std::shared_ptr<S3ClovisKVSWriter> create_clovis_kvs_writer(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> s3_clovis_api = nullptr) {
    s3_log(S3_LOG_DEBUG,
           "S3ClovisKVSWriterFactory::create_clovis_kvs_writer\n");
    return std::make_shared<S3ClovisKVSWriter>(req, s3_clovis_api);
  }
};

class S3AsyncBufferOptContainerFactory {
 public:
  virtual ~S3AsyncBufferOptContainerFactory() {}
  virtual std::shared_ptr<S3AsyncBufferOptContainer> create_async_buffer(
      size_t size_of_each_buf) {
    s3_log(S3_LOG_DEBUG,
           "S3AsyncBufferOptContainerFactory::create_async_buffer\n");
    return std::make_shared<S3AsyncBufferOptContainer>(size_of_each_buf);
  }
};

class S3PutBucketBodyFactory {
 public:
  virtual ~S3PutBucketBodyFactory() {}
  virtual std::shared_ptr<S3PutBucketBody> create_put_bucket_body(
      std::string& xml) {
    s3_log(S3_LOG_DEBUG, "S3PutBucketBodyFactory::create_put_bucket_body\n");
    return std::make_shared<S3PutBucketBody>(xml);
  }
};

class S3AuthClientFactory {
 public:
  virtual ~S3AuthClientFactory() {}
  virtual std::shared_ptr<S3AuthClient> create_auth_client(
      std::shared_ptr<S3RequestObject> request) {
    s3_log(S3_LOG_DEBUG, "S3AuthClientFactory::create_auth_client\n");
    return std::make_shared<S3AuthClient>(request);
  }
};

#endif

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
 * Original creation date: 23-Mar-2017
 */

#pragma once

#ifndef __S3_UT_MOCK_S3_FACTORY_H__
#define __S3_UT_MOCK_S3_FACTORY_H__

#include <memory>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_clovis_kvs_reader.h"
#include "mock_s3_clovis_writer.h"
#include "mock_s3_object_metadata.h"
#include "mock_s3_object_multipart_metadata.h"
#include "mock_s3_part_metadata.h"

#include "mock_s3_request_object.h"
#include "s3_factory.h"

class MockS3BucketMetadataFactory : public S3BucketMetadataFactory {
 public:
  MockS3BucketMetadataFactory(std::shared_ptr<S3RequestObject> req)
      : S3BucketMetadataFactory() {
    //  We create object here since we want to set some expectations
    // Before create_bucket_metadata_obj() is called
    mock_bucket_metadata = std::make_shared<MockS3BucketMetadata>(req);
  }

  std::shared_ptr<S3BucketMetadata> create_bucket_metadata_obj(
      std::shared_ptr<S3RequestObject> req) {
    return mock_bucket_metadata;
  }

  // Use this to setup your expectations.
  std::shared_ptr<MockS3BucketMetadata> mock_bucket_metadata;
};

class MockS3ObjectMetadataFactory : public S3ObjectMetadataFactory {
 public:
  MockS3ObjectMetadataFactory(std::shared_ptr<S3RequestObject> req,
                              m0_uint128 object_list_indx_oid)
      : S3ObjectMetadataFactory() {
    mock_object_metadata =
        std::make_shared<MockS3ObjectMetadata>(req, object_list_indx_oid);
  }

  std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 indx_oid) {
    return mock_object_metadata;
  }

  std::shared_ptr<MockS3ObjectMetadata> mock_object_metadata;
};

class MockS3PartMetadataFactory : public S3PartMetadataFactory {
 public:
  MockS3PartMetadataFactory(std::shared_ptr<S3RequestObject> req,
                            m0_uint128 indx_oid, std::string upload_id,
                            int part_num)
      : S3PartMetadataFactory() {
    mock_part_metadata = std::make_shared<MockS3PartMetadata>(
        req, indx_oid, upload_id, part_num);
  }

  std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 indx_oid,
      std::string upload_id, int part_num) {
    return mock_part_metadata;
  }

  std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, std::string upload_id,
      int part_num) {
    return mock_part_metadata;
  }

  std::shared_ptr<MockS3PartMetadata> mock_part_metadata;
};

class MockS3ObjectMultipartMetadataFactory
    : public S3ObjectMultipartMetadataFactory {
 public:
  MockS3ObjectMultipartMetadataFactory(std::shared_ptr<S3RequestObject> req,
                                       m0_uint128 mp_indx_oid, bool is_mp,
                                       std::string upload_id)
      : S3ObjectMultipartMetadataFactory() {
    //  We create object here since we want to set some expectations
    // Before create_bucket_metadata_obj() is called
    mock_object_mp_metadata = std::make_shared<MockS3ObjectMultipartMetadata>(
        req, mp_indx_oid, is_mp, upload_id);
  }

  std::shared_ptr<S3ObjectMetadata> create_object_mp_metadata_obj(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 mp_indx_oid,
      bool is_mp, std::string upload_id) {
    return mock_object_mp_metadata;
  }

  // Use this to setup your expectations.
  std::shared_ptr<MockS3ObjectMultipartMetadata> mock_object_mp_metadata;
};

class MockS3ClovisWriterFactory : public S3ClovisWriterFactory {
 public:
  MockS3ClovisWriterFactory(std::shared_ptr<S3RequestObject> req,
                            m0_uint128 oid)
      : S3ClovisWriterFactory() {
    mock_clovis_writer = std::make_shared<MockS3ClovisWriter>(req, oid);
  }

  std::shared_ptr<S3ClovisWriter> create_clovis_writer(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 oid) {
    return mock_clovis_writer;
  }

  std::shared_ptr<MockS3ClovisWriter> mock_clovis_writer;
};

class MockS3ClovisKVSReaderFactory : public S3ClovisKVSReaderFactory {
 public:
  MockS3ClovisKVSReaderFactory(std::shared_ptr<S3RequestObject> req,
                               std::shared_ptr<ClovisAPI> s3_clovis_api)
      : S3ClovisKVSReaderFactory() {
    mock_clovis_kvs_reader =
        std::make_shared<MockS3ClovisKVSReader>(req, s3_clovis_api);
  }

  std::shared_ptr<S3ClovisKVSReader> create_clovis_kvs_reader(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> s3_clovis_api) {
    return mock_clovis_kvs_reader;
  }

  std::shared_ptr<MockS3ClovisKVSReader> mock_clovis_kvs_reader;
};
#endif

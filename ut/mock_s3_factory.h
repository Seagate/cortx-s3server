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

#ifndef __S3_UT_MOCK_S3_FACTORY_H__
#define __S3_UT_MOCK_S3_FACTORY_H__

#include <memory>

#include "mock_s3_async_buffer_opt_container.h"
#include "mock_s3_auth_client.h"
#include "mock_s3_bucket_metadata.h"
#include "mock_s3_motr_kvs_reader.h"
#include "mock_s3_motr_kvs_writer.h"
#include "mock_s3_motr_reader.h"
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_motr_writer.h"
#include "mock_s3_object_metadata.h"
#include "mock_s3_object_multipart_metadata.h"
#include "mock_s3_part_metadata.h"
#include "mock_s3_put_bucket_body.h"
#include "mock_s3_request_object.h"
#include "mock_s3_put_tag_body.h"
#include "mock_s3_global_bucket_index_metadata.h"
#include "mock_s3_object_extended_metadata.h"
#include "s3_factory.h"

class MockS3BucketMetadataFactory : public S3BucketMetadataFactory {
 public:
  explicit MockS3BucketMetadataFactory(std::shared_ptr<S3RequestObject> req) {
    mock_bucket_metadata =
        std::make_shared<MockS3BucketMetadata>(std::move(req));
  }
  std::shared_ptr<S3BucketMetadata> create_bucket_metadata_obj(
      std::shared_ptr<S3RequestObject> req,
      const std::string& str_bucket_name = "") override {
    return mock_bucket_metadata;
  }
  // Use this to setup your expectations.
  std::shared_ptr<MockS3BucketMetadata> mock_bucket_metadata;
};

class MockS3ObjectMetadataFactory : public S3ObjectMetadataFactory {
 public:
  explicit MockS3ObjectMetadataFactory(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MockS3Motr> s3_motr_mock_ptr = {}) {
    mock_object_metadata =
        std::make_shared<MockS3ObjectMetadata>(req, s3_motr_mock_ptr);
    mock_object_extnd_metadata =
        std::make_shared<MockS3ObjectExtendedMetadata>(req, s3_motr_mock_ptr);
  }
  void set_object_list_index_oid(const struct m0_uint128& id) {
    mock_object_metadata->set_object_list_index_layout({id});
    mock_object_extnd_metadata->set_extended_list_index_layout({id});
  }
  void set_object_list_index_layout(struct s3_motr_idx_layout& id) {
    mock_object_metadata->set_object_list_index_layout(id);
    mock_object_extnd_metadata->set_extended_list_index_layout(id);
  }
  std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req,
      const struct s3_motr_idx_layout& obj_idx_lo = {},
      const struct s3_motr_idx_layout& obj_ver_idx_lo = {}) override {

    mock_object_metadata->set_object_list_index_layout(obj_idx_lo);
    return mock_object_metadata;
  }
  std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req, const std::string& str_bucket_name,
      const std::string& str_object_name,
      const struct s3_motr_idx_layout& obj_idx_lo = {},
      const struct s3_motr_idx_layout& obj_ver_idx_lo = {}) override {

    mock_object_metadata->set_object_list_index_layout(obj_idx_lo);
    return mock_object_metadata;
  }

  std::shared_ptr<S3ObjectExtendedMetadata> create_object_ext_metadata_obj(
      std::shared_ptr<S3RequestObject> req, const std::string& bucket_name,
      const std::string& object_name, const std::string& versionid,
      unsigned int parts = 0, unsigned int fragments = 0,
      const struct s3_motr_idx_layout& obj_idx_lo = {}) {
    mock_object_extnd_metadata->set_extended_list_index_layout(obj_idx_lo);
    mock_object_extnd_metadata->set_version_id(versionid);
    return mock_object_extnd_metadata;
  }

  std::shared_ptr<MockS3ObjectMetadata> mock_object_metadata;
  std::shared_ptr<MockS3ObjectExtendedMetadata> mock_object_extnd_metadata;
};

class MockS3PartMetadataFactory : public S3PartMetadataFactory {
 public:
  MockS3PartMetadataFactory(std::shared_ptr<S3RequestObject> req,
                            const m0_uint128& indx_oid, std::string upload_id,
                            int part_num) {
    mock_part_metadata = std::make_shared<MockS3PartMetadata>(
        req, indx_oid, upload_id, part_num);
  }
  std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, const s3_motr_idx_layout& idx_lo,
      std::string upload_id, int part_num) override {

    return mock_part_metadata;
  }
  std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, std::string upload_id,
      int part_num) override {

    return mock_part_metadata;
  }
  std::shared_ptr<MockS3PartMetadata> mock_part_metadata;
};

class MockS3ObjectMultipartMetadataFactory
    : public S3ObjectMultipartMetadataFactory {
 public:
  MockS3ObjectMultipartMetadataFactory(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<MockS3Motr> s3_motr_mock_ptr, std::string upload_id) {
    //  We create object here since we want to set some expectations
    // Before create_bucket_metadata_obj() is called
    mock_object_mp_metadata = std::make_shared<MockS3ObjectMultipartMetadata>(
        req, s3_motr_mock_ptr, upload_id);
  }
  void set_object_list_index_oid(const struct m0_uint128& id) {
    mock_object_mp_metadata->set_object_list_index_layout({id});
  }
  std::shared_ptr<S3ObjectMetadata> create_object_mp_metadata_obj(
      std::shared_ptr<S3RequestObject> req,
      const struct s3_motr_idx_layout& mp_idx_lo,
      std::string upload_id) override {

    mock_object_mp_metadata->set_object_list_index_layout(mp_idx_lo);
    return mock_object_mp_metadata;
  }
  // Use this to setup your expectations.
  std::shared_ptr<MockS3ObjectMultipartMetadata> mock_object_mp_metadata;
};

class MockS3MotrWriterFactory : public S3MotrWriterFactory {
 public:
  MockS3MotrWriterFactory(
      std::shared_ptr<RequestObject> req, m0_uint128 oid,
      std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api = {}) {
    mock_motr_writer =
        std::make_shared<MockS3MotrWiter>(req, oid, ptr_mock_s3_motr_api);
  }

  MockS3MotrWriterFactory(std::shared_ptr<RequestObject> req,
                          std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api)
      : S3MotrWriterFactory() {
    mock_motr_writer =
        std::make_shared<MockS3MotrWiter>(req, ptr_mock_s3_motr_api);
  }

  std::shared_ptr<S3MotrWiter> create_motr_writer(
      std::shared_ptr<RequestObject> req) override {
    return mock_motr_writer;
  }

  std::shared_ptr<S3MotrWiter> create_motr_writer(
      std::shared_ptr<RequestObject> req, struct m0_uint128 oid,
      struct m0_fid pv_id, uint64_t offset) override {
    return mock_motr_writer;
  }

  std::shared_ptr<MockS3MotrWiter> mock_motr_writer;
};

class MockS3MotrReaderFactory : public S3MotrReaderFactory {
 public:
  MockS3MotrReaderFactory(std::shared_ptr<RequestObject> req, m0_uint128 oid,
                          int layout_id,
                          std::shared_ptr<MockS3Motr> s3_motr_mock_apis = {}) {
    mock_motr_reader = std::make_shared<MockS3MotrReader>(req, oid, layout_id,
                                                          s3_motr_mock_apis);
  }

  std::shared_ptr<S3MotrReader> create_motr_reader(
      std::shared_ptr<RequestObject> req, struct m0_uint128 oid, int layout_id,
      struct m0_fid pvid = {},
      std::shared_ptr<MotrAPI> motr_api = {}) override {
    return mock_motr_reader;
  }

  std::shared_ptr<MockS3MotrReader> mock_motr_reader;
};

class MockS3MotrKVSReaderFactory : public S3MotrKVSReaderFactory {
 public:
  MockS3MotrKVSReaderFactory(std::shared_ptr<RequestObject> req,
                             std::shared_ptr<MockS3Motr> s3_motr_mock_api)
      : S3MotrKVSReaderFactory() {
    mock_motr_kvs_reader =
        std::make_shared<MockS3MotrKVSReader>(req, s3_motr_mock_api);
  }

  std::shared_ptr<S3MotrKVSReader> create_motr_kvs_reader(
      std::shared_ptr<RequestObject> req,
      std::shared_ptr<MotrAPI> s3_motr_api = nullptr) override {
    return mock_motr_kvs_reader;
  }

  std::shared_ptr<MockS3MotrKVSReader> mock_motr_kvs_reader;
};

class MockS3MotrKVSWriterFactory : public S3MotrKVSWriterFactory {
 public:
  MockS3MotrKVSWriterFactory(std::shared_ptr<RequestObject> req,
                             std::shared_ptr<MockS3Motr> s3_motr_api = nullptr)
      : S3MotrKVSWriterFactory() {
    mock_motr_kvs_writer =
        std::make_shared<MockS3MotrKVSWriter>(req, s3_motr_api);
  }

  std::shared_ptr<S3MotrKVSWriter> create_motr_kvs_writer(
      std::shared_ptr<RequestObject> req,
      std::shared_ptr<MotrAPI> s3_motr_api = nullptr) override {
    return mock_motr_kvs_writer;
  }

  std::shared_ptr<MockS3MotrKVSWriter> mock_motr_kvs_writer;
};

class MockS3AsyncBufferOptContainerFactory
    : public S3AsyncBufferOptContainerFactory {
 public:
  MockS3AsyncBufferOptContainerFactory(size_t size_of_each_buf)
      : S3AsyncBufferOptContainerFactory() {
    mock_async_buffer =
        std::make_shared<MockS3AsyncBufferOptContainer>(size_of_each_buf);
  }

  std::shared_ptr<S3AsyncBufferOptContainer> create_async_buffer(
      size_t size_of_each_buf) override {
    return mock_async_buffer;
  }

  std::shared_ptr<MockS3AsyncBufferOptContainer> get_mock_buffer() {
    return mock_async_buffer;
  }

  std::shared_ptr<MockS3AsyncBufferOptContainer> mock_async_buffer;
};

class MockS3PutBucketBodyFactory : public S3PutBucketBodyFactory {
 public:
  MockS3PutBucketBodyFactory(std::string& xml) : S3PutBucketBodyFactory() {
    mock_put_bucket_body = std::make_shared<MockS3PutBucketBody>(xml);
  }

  std::shared_ptr<S3PutBucketBody> create_put_bucket_body(std::string& xml)
      override {
    return mock_put_bucket_body;
  }

  // Use this to setup your expectations.
  std::shared_ptr<MockS3PutBucketBody> mock_put_bucket_body;
};

class MockS3PutTagBodyFactory : public S3PutTagsBodyFactory {
 public:
  MockS3PutTagBodyFactory(std::string& xml, std::string& request_id)
      : S3PutTagsBodyFactory() {
    mock_put_bucket_tag_body =
        std::make_shared<MockS3PutTagBody>(xml, request_id);
  }

  std::shared_ptr<S3PutTagBody> create_put_resource_tags_body(
      std::string& xml, std::string& request_id) override {
    return mock_put_bucket_tag_body;
  }

  // Use this to setup your expectations.
  std::shared_ptr<MockS3PutTagBody> mock_put_bucket_tag_body;
};

class MockS3AuthClientFactory : public S3AuthClientFactory {
 public:
  MockS3AuthClientFactory(std::shared_ptr<S3RequestObject> req)
      : S3AuthClientFactory() {
    mock_auth_client = std::make_shared<MockS3AuthClient>(req);
  }

  std::shared_ptr<S3AuthClient> create_auth_client(
      std::shared_ptr<RequestObject> req,
      bool skip_authorization = false) override {
    return mock_auth_client;
  }

  std::shared_ptr<MockS3AuthClient> mock_auth_client;
};

class MockS3GlobalBucketIndexMetadataFactory
    : public S3GlobalBucketIndexMetadataFactory {
 public:
  MockS3GlobalBucketIndexMetadataFactory(std::shared_ptr<S3RequestObject> req)
      : S3GlobalBucketIndexMetadataFactory() {
    mock_global_bucket_index_metadata =
        std::make_shared<MockS3GlobalBucketIndexMetadata>(req);
  }

  std::shared_ptr<S3GlobalBucketIndexMetadata>
  create_s3_global_bucket_index_metadata(std::shared_ptr<S3RequestObject> req,
                                         const std::string& = "",
                                         const std::string& = "",
                                         const std::string& = "") override {
    return mock_global_bucket_index_metadata;
  }

  std::shared_ptr<MockS3GlobalBucketIndexMetadata>
      mock_global_bucket_index_metadata;
};

#endif

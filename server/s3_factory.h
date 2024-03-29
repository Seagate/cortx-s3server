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

#ifndef __S3_SERVER_S3_FACTORY_H__
#define __S3_SERVER_S3_FACTORY_H__

#include "s3_global_bucket_index_metadata.h"
#include "s3_async_buffer_opt.h"
#include "s3_auth_client.h"
#include "s3_bucket_metadata_proxy.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"
#include "s3_motr_reader.h"
#include "s3_motr_writer.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_part_metadata.h"
#include "s3_put_bucket_body.h"
#include "s3_put_tag_body.h"

class S3BucketMetadataV1;

class S3BucketMetadataFactory {
 public:
  virtual ~S3BucketMetadataFactory() {}

  virtual std::shared_ptr<S3BucketMetadata> create_bucket_metadata_obj(
      std::shared_ptr<S3RequestObject> req,
      const std::string& str_bucket_name = "") {
    s3_log(S3_LOG_DEBUG, "",
           "S3BucketMetadataFactory::create_bucket_metadata_obj\n");
    return std::make_shared<S3BucketMetadataProxy>(std::move(req),
                                                   str_bucket_name);
  }
};

class S3MotrBucketMetadataFactory {
 public:
  virtual ~S3MotrBucketMetadataFactory() = default;

  virtual std::unique_ptr<S3BucketMetadataV1> create_motr_bucket_metadata_obj(
      const S3BucketMetadata& src);
};

class S3ObjectMetadataFactory {
 public:
  virtual ~S3ObjectMetadataFactory() = default;

  // The implementation is moved to s3_object_metadata.cc

  virtual std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req,
      const struct s3_motr_idx_layout& obj_idx_lo = {},
      const struct s3_motr_idx_layout& obj_ver_idx_lo = {});

  virtual std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req, const std::string& str_bucket_name,
      const std::string& str_object_name,
      const struct s3_motr_idx_layout& obj_idx_lo,
      const struct s3_motr_idx_layout& obj_ver_idx_lo);

  // Added below to create instance of 'S3ObjectExtendedMetadata' class
  virtual std::shared_ptr<S3ObjectExtendedMetadata>
      create_object_ext_metadata_obj(
          std::shared_ptr<S3RequestObject> req, const std::string& bucket_name,
          const std::string& object_name, const std::string& versionid,
          unsigned int parts, unsigned int fragments,
          const struct s3_motr_idx_layout& obj_idx_lo);
};

class S3ObjectMultipartMetadataFactory {
 public:
  virtual ~S3ObjectMultipartMetadataFactory() = default;

  // The implementation is moved to s3_object_metadata.cc

  virtual std::shared_ptr<S3ObjectMetadata> create_object_mp_metadata_obj(
      std::shared_ptr<S3RequestObject> req,
      const struct s3_motr_idx_layout& mp_idx_lo, std::string upload_id);
};

class S3PartMetadataFactory {
 public:
  virtual ~S3PartMetadataFactory() {}
  virtual std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, const s3_motr_idx_layout& idx_lo,
      std::string upload_id, int part_num) {
    s3_log(S3_LOG_DEBUG, "",
           "S3PartMetadataFactory::create_part_metadata_obj\n");
    return std::make_shared<S3PartMetadata>(std::move(req), idx_lo,
                                            std::move(upload_id), part_num);
  }
  virtual std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, std::string upload_id,
      int part_num) {
    s3_log(S3_LOG_DEBUG, "",
           "S3PartMetadataFactory::create_part_metadata_obj\n");
    return std::make_shared<S3PartMetadata>(std::move(req),
                                            std::move(upload_id), part_num);
  }
};

class S3MotrWriterFactory {
 public:
  virtual ~S3MotrWriterFactory() = default;

  virtual std::shared_ptr<S3MotrWiter> create_motr_writer(
      std::shared_ptr<RequestObject> req) {
    s3_log(S3_LOG_DEBUG, "",
           "S3MotrWriterFactory::create_motr_writer with zero offset\n");
    return std::make_shared<S3MotrWiter>(std::move(req));
  }
  virtual std::shared_ptr<S3MotrWiter> create_motr_writer(
      std::shared_ptr<RequestObject> req, struct m0_uint128 oid,
      struct m0_fid pv_id, uint64_t offset) {
    s3_log(S3_LOG_DEBUG, "",
           "S3MotrWriterFactory::create_motr_writer with offset %zu\n", offset);
    return std::make_shared<S3MotrWiter>(std::move(req), oid, pv_id, offset);
  }
};

class S3MotrReaderFactory {
 public:
  virtual ~S3MotrReaderFactory() {}

  virtual std::shared_ptr<S3MotrReader> create_motr_reader(
      std::shared_ptr<RequestObject> req, struct m0_uint128 oid, int layout_id,
      struct m0_fid pvid = {}, std::shared_ptr<MotrAPI> motr_api = {}) {
    s3_log(S3_LOG_DEBUG, "", "S3MotrReaderFactory::create_motr_reader\n");
    return std::make_shared<S3MotrReader>(std::move(req), oid, layout_id, pvid,
                                          std::move(motr_api));
  }
};

class S3MotrKVSReaderFactory {
 public:
  virtual ~S3MotrKVSReaderFactory() {}
  virtual std::shared_ptr<S3MotrKVSReader> create_motr_kvs_reader(
      std::shared_ptr<RequestObject> req,
      std::shared_ptr<MotrAPI> s3_motr_api = nullptr) {
    s3_log(S3_LOG_DEBUG, "",
           "S3MotrKVSReaderFactory::create_motr_kvs_reader\n");
    return std::make_shared<S3MotrKVSReader>(req, s3_motr_api);
  }
};

class S3MotrKVSWriterFactory {
 public:
  virtual ~S3MotrKVSWriterFactory() {}
  virtual std::shared_ptr<S3MotrKVSWriter> create_motr_kvs_writer(
      std::shared_ptr<RequestObject> req,
      std::shared_ptr<MotrAPI> s3_motr_api = nullptr) {
    s3_log(S3_LOG_DEBUG, "",
           "S3MotrKVSWriterFactory::create_motr_kvs_writer\n");
    return std::make_shared<S3MotrKVSWriter>(req, s3_motr_api);
  }
  virtual std::shared_ptr<S3MotrKVSWriter> create_sync_motr_kvs_writer(
      std::string request_id, std::shared_ptr<MotrAPI> s3_motr_api = nullptr) {
    s3_log(S3_LOG_INFO, "",
           "S3MotrKVSWriterFactory::create_sync_motr_kvs_writer\n");
    return std::make_shared<S3MotrKVSWriter>(request_id, s3_motr_api);
  }
};

class S3AsyncBufferOptContainerFactory {
 public:
  virtual ~S3AsyncBufferOptContainerFactory() {}
  virtual std::shared_ptr<S3AsyncBufferOptContainer> create_async_buffer(
      size_t size_of_each_buf) {
    s3_log(S3_LOG_DEBUG, "",
           "S3AsyncBufferOptContainerFactory::create_async_buffer\n");
    return std::make_shared<S3AsyncBufferOptContainer>(size_of_each_buf);
  }
};

class S3PutBucketBodyFactory {
 public:
  virtual ~S3PutBucketBodyFactory() {}
  virtual std::shared_ptr<S3PutBucketBody> create_put_bucket_body(
      std::string& xml) {
    s3_log(S3_LOG_DEBUG, "",
           "S3PutBucketBodyFactory::create_put_bucket_body\n");
    return std::make_shared<S3PutBucketBody>(xml);
  }
};

class S3PutTagsBodyFactory {
 public:
  virtual ~S3PutTagsBodyFactory() {}
  virtual std::shared_ptr<S3PutTagBody> create_put_resource_tags_body(
      std::string& xml, std::string& request_id) {
    s3_log(S3_LOG_DEBUG, "",
           "S3PutTagsBodyFactory::create_put_resource_tags_body\n");
    return std::make_shared<S3PutTagBody>(xml, request_id);
  }
};

class S3AuthClientFactory {
 public:
  virtual ~S3AuthClientFactory() {}
  virtual std::shared_ptr<S3AuthClient> create_auth_client(
      std::shared_ptr<RequestObject> request, bool skip_authorization = false) {
    s3_log(S3_LOG_DEBUG, "", "S3AuthClientFactory::create_auth_client\n");
    return std::make_shared<S3AuthClient>(request, skip_authorization);
  }
};

class S3GlobalBucketIndexMetadataFactory {
 public:
  virtual ~S3GlobalBucketIndexMetadataFactory() = default;

  virtual std::shared_ptr<S3GlobalBucketIndexMetadata>
  create_s3_global_bucket_index_metadata(std::shared_ptr<S3RequestObject> req,
                                         const std::string& str_bucket_name,
                                         const std::string& account_id,
                                         const std::string& account_name) {
    s3_log(S3_LOG_DEBUG, req->get_request_id(),
           "S3GlobalBucketIndexMetadataFactory::create_s3_root_bucket_index_"
           "metadata\n");
    return std::make_shared<S3GlobalBucketIndexMetadata>(
        std::move(req), str_bucket_name, account_id, account_name);
  }
};

#endif  // __S3_SERVER_S3_FACTORY_H__

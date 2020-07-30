#pragma once

#ifndef __S3_UT_MOCK_S3_PART_METADATA_H__
#define __S3_UT_MOCK_S3_PART_METADATA_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_part_metadata.h"
#include "s3_request_object.h"

class MockS3PartMetadata : public S3PartMetadata {
 public:
  MockS3PartMetadata(std::shared_ptr<S3RequestObject> req,
                     struct m0_uint128 oid, std::string uploadid, int part_num)
      : S3PartMetadata(req, oid, uploadid, part_num) {}
  MOCK_METHOD0(get_state, S3PartMetadataState());
  MOCK_METHOD1(set_md5, void(std::string));
  MOCK_METHOD0(reset_date_time_to_current, void());
  MOCK_METHOD1(set_content_length, void(std::string length));
  MOCK_METHOD1(from_json, int(std::string content));
  MOCK_METHOD2(add_user_defined_attribute,
               void(std::string key, std::string val));
  MOCK_METHOD2(load, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(save, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed));
  MOCK_METHOD2(remove_index, void(std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed));
  MOCK_METHOD2(create_index, void(std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed));
  MOCK_METHOD3(load, void(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed, int));
  MOCK_METHOD0(get_md5, std::string());
  MOCK_METHOD0(get_content_length, size_t());
  MOCK_METHOD0(get_content_length_str, std::string());
  MOCK_METHOD0(get_last_modified_iso, std::string());
  MOCK_METHOD0(get_storage_class, std::string());
  MOCK_METHOD0(get_object_name, std::string());
  MOCK_METHOD0(get_upload_id, std::string());
};

#endif

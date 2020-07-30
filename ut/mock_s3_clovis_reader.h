#pragma once

#ifndef __S3_UT_MOCK_S3_CLOVIS_READER_H__
#define __S3_UT_MOCK_S3_CLOVIS_READER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_clovis_reader.h"
#include "s3_request_object.h"

using ::testing::_;
using ::testing::Return;

class MockS3ClovisReader : public S3ClovisReader {
 public:
  MockS3ClovisReader(std::shared_ptr<RequestObject> req, struct m0_uint128 oid,
                     int layout_id,
                     std::shared_ptr<ClovisAPI> clovis_api = nullptr)
      : S3ClovisReader(req, oid, layout_id, clovis_api) {}
  MOCK_METHOD0(get_state, S3ClovisReaderOpState());
  MOCK_METHOD0(get_oid, struct m0_uint128());
  MOCK_METHOD0(get_value, std::string());
  MOCK_METHOD1(set_oid, void(struct m0_uint128 oid));
  MOCK_METHOD3(read_object_data,
               bool(size_t num_of_blocks, std::function<void(void)> on_success,
                    std::function<void(void)> on_failed));
  MOCK_METHOD1(get_first_block, size_t(char** data));
  MOCK_METHOD1(get_next_block, size_t(char** data));
};

#endif

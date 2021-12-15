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

#include <cstdlib>
#include <gtest/gtest.h>

#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"

#include "s3_data_usage.h"

// using ::testing::Return;
using ::testing::Invoke;
using ::testing::ReturnRef;
using ::testing::_;
// using ::testing::AtLeast;
// using ::testing::DefaultValue;
// using ::testing::HasSubstr;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StartsWith;

class DataUsageItemTest : public testing::Test {
 protected:
  std::shared_ptr<MockS3RequestObject> mock_request0;
  std::shared_ptr<MockS3RequestObject> mock_request1;
  std::shared_ptr<MockS3RequestObject> mock_request2;
  std::shared_ptr<MockS3Motr> mock_s3_motr_api;
  std::shared_ptr<MockS3MotrKVSReaderFactory> mock_kvs_reader_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> mock_kvs_writer_factory;
  std::string cache_key;
  std::string motr_key;
  DataUsageItemState saved_state;
  unsigned n_success_called;
  unsigned n_failure_called;
  std::shared_ptr<DataUsageItem> item_under_test;

  DataUsageItemTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr0 = new EvhtpWrapper();
    EvhtpInterface *evhtp_obj_ptr1 = new EvhtpWrapper();
    EvhtpInterface *evhtp_obj_ptr2 = new EvhtpWrapper();
    mock_request0 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr0);
    mock_request1 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr1);
    mock_request2 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr2);
    mock_s3_motr_api = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));
    mock_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
        mock_request0, mock_s3_motr_api);
    mock_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        mock_request0, mock_s3_motr_api);
    n_success_called = n_failure_called = 0;
    cache_key = "ut_item1";
    motr_key = "ut_item1/1";
    auto subscriber = std::bind(&DataUsageItemTest::item_state_changed, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3);
    item_under_test.reset(new DataUsageItem(
        mock_request0, cache_key, subscriber, mock_kvs_reader_factory,
        mock_kvs_writer_factory, mock_s3_motr_api));
  }

 public:
  void item_state_changed(DataUsageItem *item, DataUsageItemState prev,
                          DataUsageItemState curr) {
    ASSERT_TRUE(item);
    EXPECT_NE(prev, curr);
    EXPECT_EQ(curr, item->state);
    saved_state = curr;
  }

  void success_cb() { n_success_called++; }
  void failure_cb() { n_failure_called++; }
};

TEST_F(DataUsageItemTest, Constructor) {
  EXPECT_THAT(item_under_test->motr_key,
              StartsWith(item_under_test->cache_key));

  EXPECT_EQ(item_under_test->objects_count, 0);
  EXPECT_EQ(item_under_test->bytes_count, 0);

  EXPECT_TRUE(item_under_test->current_request);
  EXPECT_EQ(item_under_test->current_objects_increment, 0);
  EXPECT_EQ(item_under_test->current_bytes_increment, 0);
  EXPECT_EQ(item_under_test->current_callbacks.size(), 0);

  EXPECT_FALSE(item_under_test->pending_request);
  EXPECT_EQ(item_under_test->pending_objects_increment, 0);
  EXPECT_EQ(item_under_test->pending_bytes_increment, 0);
  EXPECT_EQ(item_under_test->pending_callbacks.size(), 0);

  EXPECT_EQ(item_under_test->state, DataUsageItemState::empty);
  EXPECT_TRUE(item_under_test->state_cb);
}

TEST_F(DataUsageItemTest, GetItemRequestId) {
  std::string item_req_id = item_under_test->get_item_request_id();
  std::string req_id = mock_request0->get_stripped_request_id();
  EXPECT_STREQ(item_req_id.c_str(), req_id.c_str());
}

TEST_F(DataUsageItemTest, SetState) {
  DataUsageItemState new_state = DataUsageItemState::active;
  item_under_test->set_state(new_state);
  EXPECT_EQ(item_under_test->state, new_state);
  EXPECT_EQ(saved_state, new_state);
}

TEST_F(DataUsageItemTest, SimpleSuccessfulSave) {
  int64_t read_objects_count = 5;
  int64_t read_bytes_count = 250;
  int64_t objects_increment = 1;
  int64_t bytes_increment = 100;
  int64_t objects_expected = read_objects_count + objects_increment;
  int64_t bytes_expected = read_bytes_count + bytes_increment;
  std::ostringstream nice_json_stream;
  nice_json_stream << "{\"objects_count\":" << read_objects_count
                   << ",\"bytes_count\":" << read_bytes_count << "}";
  std::string nice_json = nice_json_stream.str();
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(nice_json));
  std::ostringstream json_to_write_stream;
  json_to_write_stream << "{"
                        <<"\"bytes_count\":" <<bytes_expected
                        << ","
                        << "\"objects_count\":" << objects_expected
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream.str(), _, _))
      .Times(1);

  item_under_test->save(mock_request0, objects_increment, bytes_increment,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->kvs_read_success();
  item_under_test->kvs_write_success();
  EXPECT_EQ(item_under_test->objects_count, objects_expected);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
  EXPECT_EQ(n_success_called, 1);
}

TEST_F(DataUsageItemTest, SimpleSaveMissingKey) {
  int64_t objects_increment = 1;
  int64_t bytes_increment = 100;
  int64_t objects_expected = objects_increment;
  int64_t bytes_expected = bytes_increment;

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::missing));
  std::ostringstream json_to_write_stream;
  json_to_write_stream << "{"
                        <<"\"bytes_count\":" <<bytes_expected
                        << ","
                        << "\"objects_count\":" << objects_expected
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream.str(), _, _))
      .Times(1);

  item_under_test->save(mock_request0, objects_increment, bytes_increment,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->kvs_read_failure();
  item_under_test->kvs_write_success();
  EXPECT_EQ(item_under_test->objects_count, objects_expected);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
  EXPECT_EQ(n_success_called, 1);
}

TEST_F(DataUsageItemTest, SimpleSaveFailedToRead) {
  int64_t objects_increment = 1;
  int64_t bytes_increment = 100;

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::failed));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, _, _, _)).Times(0);

  item_under_test->save(mock_request0, objects_increment, bytes_increment,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->kvs_read_failure();
  EXPECT_EQ(item_under_test->objects_count, 0);
  EXPECT_EQ(item_under_test->bytes_count, 0);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::failed);
  EXPECT_EQ(saved_state, DataUsageItemState::failed);
  EXPECT_EQ(n_failure_called, 1);
}

TEST_F(DataUsageItemTest, SimpleSaveFailedToWrite) {
  int64_t read_objects_count = 5;
  int64_t read_bytes_count = 250;
  int64_t objects_increment = 1;
  int64_t bytes_increment = 100;
  int64_t objects_expected = read_objects_count + objects_increment;
  int64_t bytes_expected = read_bytes_count + bytes_increment;
  std::ostringstream nice_json_stream;
  nice_json_stream << "{\"objects_count\":" << read_objects_count
                   << ",\"bytes_count\":" << read_bytes_count << "}";
  std::string nice_json = nice_json_stream.str();
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(nice_json));
  std::ostringstream json_to_write_stream;
  json_to_write_stream << "{"
                        <<"\"bytes_count\":" << bytes_expected
                        << ","
                        << "\"objects_count\":" << objects_expected
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream.str(), _, _))
      .Times(1);

  item_under_test->save(mock_request0, objects_increment, bytes_increment,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->kvs_read_success();
  item_under_test->kvs_write_failure();
  EXPECT_EQ(item_under_test->objects_count, read_objects_count);
  EXPECT_EQ(item_under_test->bytes_count, read_bytes_count);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
  EXPECT_EQ(n_failure_called, 1);
}

TEST_F(DataUsageItemTest, MultipleSaveSuccessful) {
  int64_t read_objects_count = 5;
  int64_t read_bytes_count = 250;
  int64_t objects_increment0 = 1;
  int64_t bytes_increment0 = 11;
  int64_t objects_increment1 = 2;
  int64_t bytes_increment1 = 22;
  int64_t objects_expected1 =
      read_objects_count + objects_increment0 + objects_increment1;
  int64_t bytes_expected1 =
      read_bytes_count + bytes_increment0 + bytes_increment1;
  int64_t objects_increment2 = 5;
  int64_t bytes_increment2 = 50;
  int64_t objects_expected2 = objects_expected1 + objects_increment2;
  int64_t bytes_expected2 = bytes_expected1 + bytes_increment2;
  std::ostringstream nice_json_stream;
  nice_json_stream << "{\"objects_count\":" << read_objects_count
                   << ",\"bytes_count\":" << read_bytes_count << "}";
  std::string nice_json = nice_json_stream.str();

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(nice_json));

  std::ostringstream json_to_write_stream1;
  json_to_write_stream1 << "{"
                        <<"\"bytes_count\":" << bytes_expected1
                        << ","
                        << "\"objects_count\":" << objects_expected1
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream1.str(), _, _))
      .Times(1);

  std::ostringstream json_to_write_stream2;
  json_to_write_stream2 << "{"
                        <<"\"bytes_count\":" << bytes_expected2
                        << ","
                        << "\"objects_count\":" << objects_expected2
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream2.str(), _, _))
      .Times(1);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->save(mock_request1, objects_increment1, bytes_increment1,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request0->get_stripped_request_id().c_str());
  item_under_test->kvs_read_success();
  item_under_test->save(mock_request2, objects_increment2, bytes_increment2,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request0->get_stripped_request_id().c_str());
  item_under_test->kvs_write_success();
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request2->get_stripped_request_id().c_str());
  item_under_test->kvs_write_success();
  EXPECT_EQ(n_success_called, 3);
  EXPECT_EQ(item_under_test->objects_count, objects_expected2);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected2);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
}

TEST_F(DataUsageItemTest, MultipleSaveOneWriteFailed) {
  int64_t read_objects_count = 5;
  int64_t read_bytes_count = 250;
  int64_t objects_increment0 = 1;
  int64_t bytes_increment0 = 10;
  int64_t objects_increment1 = 2;
  int64_t bytes_increment1 = 20;
  int64_t objects_expected1 =
      read_objects_count + objects_increment0 + objects_increment1;
  int64_t bytes_expected1 =
      read_bytes_count + bytes_increment0 + bytes_increment1;
  int64_t objects_increment2 = 5;
  int64_t bytes_increment2 = 50;
  int64_t objects_expected2 = read_objects_count + objects_increment2;
  int64_t bytes_expected2 = read_bytes_count + bytes_increment2;
  std::ostringstream nice_json_stream;
  nice_json_stream << "{\"objects_count\":" << read_objects_count
                   << ",\"bytes_count\":" << read_bytes_count << "}";
  std::string nice_json = nice_json_stream.str();

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(nice_json));

  std::ostringstream json_to_write_stream1;
  json_to_write_stream1 << "{"
                        <<"\"bytes_count\":" << bytes_expected1
                        << ","
                        << "\"objects_count\":" << objects_expected1
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream1.str(), _, _))
      .Times(1);

  std::ostringstream json_to_write_stream2;
  json_to_write_stream2 << "{"
                        <<"\"bytes_count\":" << bytes_expected2
                        << ","
                        << "\"objects_count\":" << objects_expected2
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream2.str(), _, _))
      .Times(1);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->save(mock_request1, objects_increment1, bytes_increment1,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request0->get_stripped_request_id().c_str());
  item_under_test->kvs_read_success();
  item_under_test->save(mock_request2, objects_increment2, bytes_increment2,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request0->get_stripped_request_id().c_str());
  item_under_test->kvs_write_failure();
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request2->get_stripped_request_id().c_str());
  item_under_test->kvs_write_success();
  EXPECT_EQ(n_failure_called, 2);
  EXPECT_EQ(n_success_called, 1);
  EXPECT_EQ(item_under_test->objects_count, objects_expected2);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected2);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
}

TEST_F(DataUsageItemTest, MultipleSaveConsequent) {
  int64_t read_objects_count = 5;
  int64_t read_bytes_count = 250;
  int64_t objects_increment0 = 1;
  int64_t bytes_increment0 = 11;
  int64_t objects_increment1 = 2;
  int64_t bytes_increment1 = 22;
  int64_t objects_expected1 =
      read_objects_count + objects_increment0 + objects_increment1;
  int64_t bytes_expected1 =
      read_bytes_count + bytes_increment0 + bytes_increment1;
  int64_t objects_increment2 = 3;
  int64_t bytes_increment2 = 33;
  int64_t objects_expected2 = objects_expected1 + objects_increment2;
  int64_t bytes_expected2 = bytes_expected1 + bytes_increment2;
  std::ostringstream nice_json_stream;
  nice_json_stream << "{\"objects_count\":" << read_objects_count
                   << ",\"bytes_count\":" << read_bytes_count << "}";
  std::string nice_json = nice_json_stream.str();

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(nice_json));

  std::ostringstream json_to_write_stream1;
  json_to_write_stream1 << "{"
                        <<"\"bytes_count\":" << bytes_expected1
                        << ","
                        << "\"objects_count\":" << objects_expected1
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream1.str(), _, _))
      .Times(1);

  std::ostringstream json_to_write_stream2;
  json_to_write_stream2 << "{"
                        <<"\"bytes_count\":" << bytes_expected2
                        << ","
                        << "\"objects_count\":" << objects_expected2
                        << "}\n";
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, motr_key, json_to_write_stream2.str(), _, _))
      .Times(1);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->save(mock_request1, objects_increment1, bytes_increment1,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request0->get_stripped_request_id().c_str());
  item_under_test->kvs_read_success();
  item_under_test->kvs_write_success();
  EXPECT_EQ(item_under_test->objects_count, objects_expected1);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected1);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
  item_under_test->save(mock_request2, objects_increment2, bytes_increment2,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request2->get_stripped_request_id().c_str());
  item_under_test->kvs_write_success();
  EXPECT_EQ(n_success_called, 3);
  EXPECT_EQ(item_under_test->objects_count, objects_expected2);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected2);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
}

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
using ::testing::Ne;
using ::testing::StrEq;
using ::testing::StartsWith;

class DataUsageItemTest : public testing::Test {
 protected:
  std::shared_ptr<MockS3RequestObject> mock_request0;
  int64_t objects_increment0;
  int64_t bytes_increment0;
  int64_t objects_expected0;
  int64_t bytes_expected0;
  std::string json_to_write0;
  std::shared_ptr<MockS3RequestObject> mock_request1;
  int64_t objects_increment1;
  int64_t bytes_increment1;
  int64_t objects_expected01;
  int64_t bytes_expected01;
  std::string json_to_write01;
  std::shared_ptr<MockS3RequestObject> mock_request2;
  int64_t objects_increment2;
  int64_t bytes_increment2;
  int64_t objects_expected012;
  int64_t bytes_expected012;
  int64_t objects_expected2;
  int64_t bytes_expected2;
  std::string json_to_write012;
  std::string json_to_write2;
  std::shared_ptr<MockS3Motr> mock_s3_motr_api;
  std::shared_ptr<MockS3MotrKVSReaderFactory> mock_kvs_reader_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> mock_kvs_writer_factory;
  DataUsageItemState saved_state;
  unsigned n_success_called;
  unsigned n_failure_called;
  std::string item_cache_key;
  std::string item_motr_key;
  std::shared_ptr<DataUsageItem> item_under_test;

  int64_t mock_motr_objects_count;
  int64_t mock_motr_bytes_count;
  std::string mock_motr_json;

 public:
  DataUsageItemTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr0 = new EvhtpWrapper();
    EvhtpInterface *evhtp_obj_ptr1 = new EvhtpWrapper();
    EvhtpInterface *evhtp_obj_ptr2 = new EvhtpWrapper();
    mock_request0 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr0);
    mock_motr_objects_count = 5;
    mock_motr_bytes_count = 250;
    mock_motr_json =
        create_json_string(mock_motr_objects_count, mock_motr_bytes_count);
    objects_increment0 = 1;
    bytes_increment0 = 10;
    objects_expected0 = mock_motr_objects_count + objects_increment0;
    bytes_expected0 = mock_motr_bytes_count + bytes_increment0;
    json_to_write0 = create_json_string(objects_expected0, bytes_expected0);
    mock_request1 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr1);
    objects_increment1 = 2;
    bytes_increment1 = 20;
    objects_expected01 = objects_expected0 + objects_increment1;
    bytes_expected01 = bytes_expected0 + bytes_increment1;
    json_to_write01 = create_json_string(objects_expected01, bytes_expected01);
    mock_request2 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr2);
    objects_increment2 = 5;
    bytes_increment2 = 50;
    objects_expected012 = objects_expected01 + objects_increment2;
    bytes_expected012 = bytes_expected01 + bytes_increment2;
    json_to_write012 =
        create_json_string(objects_expected012, bytes_expected012);
    objects_expected2 = mock_motr_objects_count + objects_increment2;
    bytes_expected2 = mock_motr_bytes_count + bytes_increment2;
    json_to_write2 = create_json_string(objects_expected2, bytes_expected2);
    mock_s3_motr_api = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));
    mock_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
        mock_request0, mock_s3_motr_api);
    mock_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        mock_request0, mock_s3_motr_api);
    n_success_called = n_failure_called = 0;

    item_cache_key = "ut_item1";
    item_motr_key = "ut_item1/1";
    auto subscriber = std::bind(&DataUsageItemTest::item_state_changed, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3);
    item_under_test.reset(new DataUsageItem(
        mock_request0, item_cache_key, subscriber, mock_kvs_reader_factory,
        mock_kvs_writer_factory, mock_s3_motr_api));
  }

  void item_state_changed(DataUsageItem *item, DataUsageItemState prev,
                          DataUsageItemState curr) {
    ASSERT_TRUE(item);
    EXPECT_NE(prev, curr);
    EXPECT_EQ(curr, item->state);
    saved_state = curr;
  }

  std::string create_json_string(int64_t objects_count, int64_t bytes_count) {
    std::ostringstream json_stream;
    json_stream << "{"
                << "\"bytes_count\":" << bytes_count << ","
                << "\"objects_count\":" << objects_count << "}" << std::endl;
    return json_stream.str();
  }

  void success_cb() { n_success_called++; }
  void failure_cb() { n_failure_called++; }
};

class MockDataUsageItemFactory : public DataUsageItemFactory {
 public:
  virtual ~MockDataUsageItemFactory() {}
  virtual std::shared_ptr<DataUsageItem> create_data_usage_item(
      std::shared_ptr<RequestObject> req, const std::string &key_in_cache,
      std::function<void(DataUsageItem *, DataUsageItemState,
                         DataUsageItemState)> subscriber) override {
    std::shared_ptr<MockS3Motr> mock_s3_motr_api =
        std::make_shared<MockS3Motr>();
    //EXPECT_CALL(*mock_s3_motr_api, m0_h_ufid_next(_))
    //    .WillRepeatedly(Invoke(dummy_helpers_ufid_next));
    std::shared_ptr<MockS3MotrKVSReaderFactory> mock_kvs_reader_factory =
        std::make_shared<MockS3MotrKVSReaderFactory>(req, mock_s3_motr_api);
    std::shared_ptr<MockS3MotrKVSWriterFactory> mock_kvs_writer_factory =
        std::make_shared<MockS3MotrKVSWriterFactory>(req, mock_s3_motr_api);
    return std::make_shared<DataUsageItem>(
        req, key_in_cache, subscriber, mock_kvs_reader_factory,
        mock_kvs_writer_factory, mock_s3_motr_api);
  }
};

class S3DataUsageCacheTest : public testing::Test {
 protected:
  std::shared_ptr<MockS3RequestObject> mock_request0;
  std::string test_account_name0;
  int64_t objects_increment0;
  int64_t bytes_increment0;
  std::shared_ptr<MockS3RequestObject> mock_request1;
  std::string test_account_name1;
  int64_t objects_increment1;
  int64_t bytes_increment1;
  S3DataUsageCache *cache_under_test;

 public:
  S3DataUsageCacheTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr0 = new EvhtpWrapper();
    EvhtpInterface *evhtp_obj_ptr1 = new EvhtpWrapper();
    mock_request0 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr0);
    test_account_name0 = "ut_account0";
    objects_increment0 = 1;
    bytes_increment0 = 10;
    mock_request0->set_account_name(test_account_name0);
    mock_request1 = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr1);
    test_account_name1 = "ut_account1";
    objects_increment0 = 2;
    bytes_increment0 = 20;
    mock_request1->set_account_name(test_account_name1);
    cache_under_test = S3DataUsageCache::get_instance();
    std::shared_ptr<MockDataUsageItemFactory> mock_item_factory =
        std::make_shared<MockDataUsageItemFactory>();
    cache_under_test->set_max_cache_size(1);
    cache_under_test->set_item_factory(mock_item_factory);
  }

  void cb() {};
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
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(mock_motr_json));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write0, _, _, _)).Times(1);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->kvs_read_success();
  item_under_test->kvs_write_success();
  EXPECT_EQ(item_under_test->objects_count, objects_expected0);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected0);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
  EXPECT_EQ(n_success_called, 1);
}

TEST_F(DataUsageItemTest, SimpleSaveMissingKey) {
  int64_t objects_expected = objects_increment0;
  int64_t bytes_expected = bytes_increment0;

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::missing));
  std::string json_to_write =
      create_json_string(objects_expected, bytes_expected);
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write, _, _, _)).Times(1);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
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
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::failed));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, _, _, _, _)).Times(0);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
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
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(mock_motr_json));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write0, _, _, _)).Times(1);

  item_under_test->save(mock_request0, objects_increment0, bytes_increment0,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));
  item_under_test->kvs_read_success();
  item_under_test->kvs_write_failure();

  EXPECT_EQ(item_under_test->objects_count, mock_motr_objects_count);
  EXPECT_EQ(item_under_test->bytes_count, mock_motr_bytes_count);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
  EXPECT_EQ(n_failure_called, 1);
}

TEST_F(DataUsageItemTest, MultipleSaveSuccessful) {
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(mock_motr_json));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write01, _, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write012, _, _, _)).Times(1);

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
  EXPECT_EQ(item_under_test->objects_count, objects_expected012);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected012);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
}

TEST_F(DataUsageItemTest, MultipleSaveOneWriteFailed) {
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(mock_motr_json));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write01, _, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write2, _, _, _)).Times(1);

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
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, item_motr_key, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_value())
      .Times(1)
      .WillRepeatedly(ReturnRef(mock_motr_json));
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write01, _, _, _)).Times(1);
  EXPECT_CALL(*(mock_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, item_motr_key, json_to_write012, _, _, _)).Times(1);

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

  EXPECT_EQ(item_under_test->objects_count, objects_expected01);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected01);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);

  item_under_test->save(mock_request2, objects_increment2, bytes_increment2,
                        std::bind(&DataUsageItemTest::success_cb, this),
                        std::bind(&DataUsageItemTest::failure_cb, this));

  EXPECT_STREQ(item_under_test->get_item_request_id().c_str(),
               mock_request2->get_stripped_request_id().c_str());

  item_under_test->kvs_write_success();

  EXPECT_EQ(n_success_called, 3);
  EXPECT_EQ(item_under_test->objects_count, objects_expected012);
  EXPECT_EQ(item_under_test->bytes_count, bytes_expected012);
  EXPECT_EQ(item_under_test->state, DataUsageItemState::inactive);
  EXPECT_EQ(saved_state, DataUsageItemState::inactive);
}

TEST_F(S3DataUsageCacheTest, SetMaxCacheSize) {
  unsigned new_size = 2;
  cache_under_test->set_max_cache_size(new_size);
  EXPECT_EQ(cache_under_test->max_cache_size, new_size);
}

TEST_F(S3DataUsageCacheTest, UpdateWithEmptyCache) {
  S3DataUsageCache::update_data_usage(
      mock_request0, nullptr, objects_increment0, bytes_increment0,
      std::bind(&S3DataUsageCacheTest::cb, this),
      std::bind(&S3DataUsageCacheTest::cb, this));
  EXPECT_NE(cache_under_test->items.find(test_account_name0),
            cache_under_test->items.end());
}

TEST_F(S3DataUsageCacheTest, UpdateWithFullCacheNoEviction) {
  S3DataUsageCache::update_data_usage(
      mock_request0, nullptr, objects_increment0, bytes_increment0,
      std::bind(&S3DataUsageCacheTest::cb, this),
      std::bind(&S3DataUsageCacheTest::cb, this));
  S3DataUsageCache::update_data_usage(
      mock_request1, nullptr, objects_increment1, bytes_increment1,
      std::bind(&S3DataUsageCacheTest::cb, this),
      std::bind(&S3DataUsageCacheTest::cb, this));
  EXPECT_NE(cache_under_test->items.find(test_account_name0),
            cache_under_test->items.end());
  EXPECT_EQ(cache_under_test->items.find(test_account_name1),
            cache_under_test->items.end());
}

TEST_F(S3DataUsageCacheTest, UpdateWithFullCacheEviction) {
  S3DataUsageCache::update_data_usage(
      mock_request0, nullptr, objects_increment0, bytes_increment0,
      std::bind(&S3DataUsageCacheTest::cb, this),
      std::bind(&S3DataUsageCacheTest::cb, this));
  ASSERT_NE(cache_under_test->items.find(test_account_name0),
            cache_under_test->items.end());
  cache_under_test->items[test_account_name0]
      ->set_state(DataUsageItemState::inactive);
  S3DataUsageCache::update_data_usage(
      mock_request1, nullptr, objects_increment1, bytes_increment1,
      std::bind(&S3DataUsageCacheTest::cb, this),
      std::bind(&S3DataUsageCacheTest::cb, this));
  EXPECT_EQ(cache_under_test->items.find(test_account_name0),
            cache_under_test->items.end());
  EXPECT_NE(cache_under_test->items.find(test_account_name1),
            cache_under_test->items.end());
}

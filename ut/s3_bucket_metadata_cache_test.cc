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
#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// server headers
#include "evhtp_wrapper.h"
#include "s3_bucket_metadata_cache.h"
#include "s3_bucket_metadata_proxy.h"
#include "s3_bucket_metadata_v1.h"
#include "s3_factory.h"

// UT headers
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"

class S3BucketMetadataV1Mock : public S3BucketMetadataV1 {
 public:
  explicit S3BucketMetadataV1Mock(const S3BucketMetadata& src);
  ~S3BucketMetadataV1Mock() override;

  void load(const S3BucketMetadata& src, CallbackHandlerType callback) override;
  void save(const S3BucketMetadata& src, CallbackHandlerType callback) override;
  void update(const S3BucketMetadata& src,
              CallbackHandlerType callback) override;
  void remove(CallbackHandlerType callback) override;

  void done(S3BucketMetadataState state);

  static unsigned n_called;

 private:
  CallbackHandlerType cb;
};

unsigned S3BucketMetadataV1Mock::n_called;
S3BucketMetadataV1Mock* p_s3_bucket_metadata_v1_test;

S3BucketMetadataV1Mock::S3BucketMetadataV1Mock(const S3BucketMetadata& src)
    : S3BucketMetadataV1(src) {
  p_s3_bucket_metadata_v1_test = this;
}

S3BucketMetadataV1Mock::~S3BucketMetadataV1Mock() {
  p_s3_bucket_metadata_v1_test = nullptr;
}
void S3BucketMetadataV1Mock::load(const S3BucketMetadata& src,
                                  CallbackHandlerType callback) {
  cb = std::move(callback);
  ++n_called;
}

void S3BucketMetadataV1Mock::save(const S3BucketMetadata& src,
                                  CallbackHandlerType callback) {
  cb = std::move(callback);
  ++n_called;
}

void S3BucketMetadataV1Mock::update(const S3BucketMetadata& src,
                                    CallbackHandlerType callback) {
  cb = std::move(callback);
  ++n_called;
}

void S3BucketMetadataV1Mock::remove(CallbackHandlerType callback) {
  cb = std::move(callback);
  ++n_called;
}

void S3BucketMetadataV1Mock::done(S3BucketMetadataState state) {
  set_state(state);

  ASSERT_TRUE(cb);
  auto cb = std::move(this->cb);
  cb(state);
}

class S3MotrBucketMetadataFactoryTest : public S3MotrBucketMetadataFactory {
 public:
  std::unique_ptr<S3BucketMetadataV1> create_motr_bucket_metadata_obj(
      const S3BucketMetadata& src) override;
};

std::unique_ptr<S3BucketMetadataV1>
S3MotrBucketMetadataFactoryTest::create_motr_bucket_metadata_obj(
    const S3BucketMetadata& src) {

  return std::unique_ptr<S3BucketMetadataV1>(new S3BucketMetadataV1Mock(src));
}

class S3BucketMetadataCacheTest : public testing::Test {
 public:
  using CurrentOp = S3BucketMetadataCache::Item::CurrentOp;

 protected:
  S3BucketMetadataCacheTest();
  void TearDown() override;
  std::unique_ptr<S3BucketMetadataProxy> create_proxy(
      const std::string& bucket_name);

  size_t get_cache_size() const;
  CurrentOp get_current_op(const std::string& bucket_name) const;

  std::function<void(void)> success_handler;
  std::function<void(void)> failed_handler;

  bool f_success = false;
  bool f_fail = false;

 private:
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<S3MotrBucketMetadataFactory> ptr_motr_bucket_metadata_factory;
  std::unique_ptr<S3BucketMetadataCache> ptr_bucket_metadata_cache;

  void on_success() { f_success = true; }
  void on_failed() { f_fail = true; }
};

#define MAX_CACHE_SIZE 2
#define EXPIRE_SEC 2
#define REFRESH_SEC 1

S3BucketMetadataCacheTest::S3BucketMetadataCacheTest() {

  success_handler = std::bind(&S3BucketMetadataCacheTest::on_success, this);
  failed_handler = std::bind(&S3BucketMetadataCacheTest::on_failed, this);

  ptr_mock_request =
      std::make_shared<MockS3RequestObject>(nullptr, new EvhtpWrapper());

  ptr_mock_request->set_account_name("s3account");
  ptr_mock_request->set_account_id("s3accountid");
  ptr_mock_request->set_user_name("s3user");
  ptr_mock_request->set_user_id("s3userid");

  ptr_motr_bucket_metadata_factory =
      std::make_shared<S3MotrBucketMetadataFactoryTest>();

  ptr_bucket_metadata_cache.reset(
      new S3BucketMetadataCache(MAX_CACHE_SIZE, EXPIRE_SEC, REFRESH_SEC,
                                ptr_motr_bucket_metadata_factory));
}

void S3BucketMetadataCacheTest::TearDown() {
  // clear the cache
  for (auto n = get_cache_size(); n > 0; --n) {
    S3BucketMetadataCache::get_instance()->shrink();
  }
  S3BucketMetadataV1Mock::n_called = 0;
}

std::unique_ptr<S3BucketMetadataProxy> S3BucketMetadataCacheTest::create_proxy(
    const std::string& bucket_name) {

  return std::unique_ptr<S3BucketMetadataProxy>(
      new S3BucketMetadataProxy(ptr_mock_request, bucket_name));
}

size_t S3BucketMetadataCacheTest::get_cache_size() const {
  return S3BucketMetadataCache::get_instance()->items.size();
}

S3BucketMetadataCacheTest::CurrentOp S3BucketMetadataCacheTest::get_current_op(
    const std::string& bucket_name) const {

  auto it = S3BucketMetadataCache::get_instance()->items.find(bucket_name);
  assert(it != S3BucketMetadataCache::get_instance()->items.end());

  return it->second->current_op;
}

// ************************************************************************* //
// ********************************* TESTS ********************************* //

TEST_F(S3BucketMetadataCacheTest, BucketFullLifeCycle) {
  // Hope, the test will last less than 2 seconds
  std::string bucket_name = "seagatebucket";

  auto ptr_metadata_proxy_1 = create_proxy(bucket_name);
  ptr_metadata_proxy_1->load(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::fetching);
  EXPECT_STREQ(p_s3_bucket_metadata_v1_test->get_bucket_name().c_str(),
               bucket_name.c_str());

  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::missing);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_TRUE(f_fail);
  f_fail = false;
  EXPECT_EQ(ptr_metadata_proxy_1->get_state(), S3BucketMetadataState::missing);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::none);
  EXPECT_EQ(get_cache_size(), 1);

  // We are planning to "create" bucket
  ptr_metadata_proxy_1->initialize();
  ptr_metadata_proxy_1->save(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 2);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::saving);

  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::present);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_TRUE(f_success);
  f_success = false;
  EXPECT_EQ(ptr_metadata_proxy_1->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::none);
  EXPECT_EQ(get_cache_size(), 1);

  ptr_metadata_proxy_1.reset();
  S3BucketMetadataV1Mock::n_called = 0;

  // We are planning to "update" bucket metadata
  auto ptr_metadata_proxy_2 = create_proxy(bucket_name);
  ptr_metadata_proxy_2->load(success_handler, failed_handler);

  // The cache MUST return already cached value
  ASSERT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 0);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::none);
  EXPECT_EQ(ptr_metadata_proxy_2->get_state(), S3BucketMetadataState::present);
  EXPECT_TRUE(f_success);
  f_success = false;

  ptr_metadata_proxy_2->setpolicy("Figvam");
  ptr_metadata_proxy_2->update(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::saving);

  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::present);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_TRUE(f_success);
  f_success = false;
  EXPECT_EQ(ptr_metadata_proxy_2->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::none);
  EXPECT_EQ(get_cache_size(), 1);

  ptr_metadata_proxy_2.reset();
  S3BucketMetadataV1Mock::n_called = 0;

  // Now we are planning to "remove" the bucket
  auto ptr_metadata_proxy_3 = create_proxy(bucket_name);
  ptr_metadata_proxy_3->load(success_handler, failed_handler);

  // The cache MUST return already cached value
  ASSERT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 0);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::none);
  EXPECT_EQ(ptr_metadata_proxy_3->get_state(), S3BucketMetadataState::present);
  EXPECT_TRUE(f_success);
  f_success = false;

  ptr_metadata_proxy_3->remove(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);
  EXPECT_EQ(get_current_op(bucket_name), CurrentOp::deleting);

  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::missing);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_TRUE(f_success);
  f_success = false;
  EXPECT_EQ(ptr_metadata_proxy_3->get_state(), S3BucketMetadataState::missing);
  EXPECT_EQ(get_cache_size(), 0);
}

TEST_F(S3BucketMetadataCacheTest, LoadQueue) {
  ASSERT_EQ(get_cache_size(), 0);
  std::string bucket_name = "seagatebucket";

  auto ptr_metadata_proxy_1 = create_proxy(bucket_name);
  ptr_metadata_proxy_1->load(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);

  auto ptr_metadata_proxy_2 = create_proxy(bucket_name);
  ptr_metadata_proxy_2->load(success_handler, failed_handler);

  auto ptr_metadata_proxy_3 = create_proxy(bucket_name);
  ptr_metadata_proxy_3->load(success_handler, failed_handler);

  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);
  EXPECT_EQ(ptr_metadata_proxy_1->get_state(), S3BucketMetadataState::empty);
  EXPECT_EQ(ptr_metadata_proxy_2->get_state(), S3BucketMetadataState::empty);
  EXPECT_EQ(ptr_metadata_proxy_3->get_state(), S3BucketMetadataState::empty);

  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::present);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);

  EXPECT_EQ(ptr_metadata_proxy_1->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(ptr_metadata_proxy_2->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(ptr_metadata_proxy_3->get_state(), S3BucketMetadataState::present);
}

TEST_F(S3BucketMetadataCacheTest, ExpirationTime) {
  ASSERT_EQ(get_cache_size(), 0);
  std::string bucket_name = "seagatebucket";

  auto ptr_metadata_proxy_1 = create_proxy(bucket_name);
  ptr_metadata_proxy_1->load(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);

  // Simulate, that the bucket exists
  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::present);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_TRUE(f_success);
  f_success = false;
  EXPECT_EQ(ptr_metadata_proxy_1->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(get_cache_size(), 1);

  ptr_metadata_proxy_1.reset();
  S3BucketMetadataV1Mock::n_called = 0;

  auto ptr_metadata_proxy_2 = create_proxy(bucket_name);
  ptr_metadata_proxy_2->load(success_handler, failed_handler);

  EXPECT_TRUE(f_success);
  f_success = false;
  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 0);
  EXPECT_EQ(ptr_metadata_proxy_2->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(get_cache_size(), 1);

  ptr_metadata_proxy_2.reset();

  std::cout << "Waining for " << EXPIRE_SEC << " seconds... " << std::flush;
  std::this_thread::sleep_for(
      std::chrono::milliseconds(EXPIRE_SEC * 1000 + 10));
  std::cout << "Done" << std::endl;

  auto ptr_metadata_proxy_3 = create_proxy(bucket_name);
  ptr_metadata_proxy_3->load(success_handler, failed_handler);

  ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
  EXPECT_EQ(S3BucketMetadataV1Mock::n_called, 1);

  // Simulate, that the bucket exists
  p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::present);

  EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
  EXPECT_TRUE(f_success);
  f_success = false;
  EXPECT_EQ(ptr_metadata_proxy_3->get_state(), S3BucketMetadataState::present);
  EXPECT_EQ(get_cache_size(), 1);
}

TEST_F(S3BucketMetadataCacheTest, CacheSize) {
  ASSERT_EQ(get_cache_size(), 0);

  for (unsigned i = 0; i < MAX_CACHE_SIZE * 2;) {
    auto bucket_name = "seagatebucket_" + std::to_string(i);

    auto ptr_metadata_proxy = create_proxy(bucket_name);
    ptr_metadata_proxy->load(success_handler, failed_handler);

    ASSERT_TRUE(p_s3_bucket_metadata_v1_test);
    EXPECT_EQ(S3BucketMetadataV1Mock::n_called, ++i);

    p_s3_bucket_metadata_v1_test->done(S3BucketMetadataState::present);

    EXPECT_FALSE(p_s3_bucket_metadata_v1_test);
    EXPECT_TRUE(f_success);
    f_success = false;
  }
  EXPECT_EQ(get_cache_size(), MAX_CACHE_SIZE);
}

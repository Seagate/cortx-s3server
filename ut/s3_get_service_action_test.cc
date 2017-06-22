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
 * Original author:  Swapnil Belapurkar  <swapnil.belapurkar@seagate.com>
 * Original creation date: April-27-2017
 */

#include <memory>

#include "mock_s3_account_user_idx_metadata.h"
#include "mock_s3_bucket_metadata.h"
#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_factory.h"
#include "s3_error_codes.h"
#include "s3_get_service_action.h"
#include "s3_test_utils.h"

using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::StrEq;
using ::testing::AtLeast;

class S3GetServiceActionTest : public testing::Test {
 protected:
  S3GetServiceActionTest() {
    S3Option::get_instance()->disable_auth();

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    zero_oid_idx = {0ULL, 0ULL};

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());
    // Mock objects.
    ptr_mock_request = std::make_shared<MockS3RequestObject>(
        req, evhtp_obj_ptr, async_buffer_factory);
    s3_clovis_api_mock = std::make_shared<MockS3Clovis>();

    // Mock factories.
    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        ptr_mock_request, s3_clovis_api_mock);
    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);
    user_idx_md_factory =
        std::make_shared<MockS3AccountUserIdxMetadataFactory>(ptr_mock_request);

    // Object to be tested.
    action_under_test.reset(
        new S3GetServiceAction(ptr_mock_request, clovis_kvs_reader_factory,
                               bucket_meta_factory, user_idx_md_factory));
  }

  std::shared_ptr<ClovisAPI> s3_clovis_api_mock;
  std::shared_ptr<S3GetServiceAction> action_under_test;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3AccountUserIdxMetadataFactory> user_idx_md_factory;
  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 oid;
  struct m0_uint128 zero_oid_idx;
  std::map<std::string, std::pair<int, std::string>> result_keys_values;
};

// Test that checks if fields of S3GetServiceAction object have been initialized
// to their default value.
TEST_F(S3GetServiceActionTest, ConstructorTest) {
  // Check value of OID.
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->bucket_list_index_oid);
  // Number of tasks in task list should not be zero.
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

// Verify that fetch bucket list indeed calls get_bucket_list_index_oid if user
// index metadata is present.
TEST_F(S3GetServiceActionTest,
       GetNextBucketCallsGetBucketListIndexIfMetadataPresent) {
  EXPECT_CALL(*(user_idx_md_factory->mock_account_user_index_metadata),
              get_state())
      .Times(1)
      .WillOnce(Return(S3AccountUserIdxMetadataState::present));
  user_idx_md_factory->mock_account_user_index_metadata
      ->set_bucket_list_index_oid(zero_oid_idx);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(AtLeast(1));
  action_under_test->account_user_index_metadata =
      user_idx_md_factory->mock_account_user_index_metadata;
  action_under_test->get_next_buckets();
}

// Verify that fetch bucket list does not call get_bucket_list_index_oid if user
// index metadata is failed.
TEST_F(S3GetServiceActionTest,
       GetNextBucketDoesNotCallsGetBucketListIndexIfMetadataFailed) {
  EXPECT_CALL(*(user_idx_md_factory->mock_account_user_index_metadata),
              get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3AccountUserIdxMetadataState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(S3HttpFailed500, _))
      .Times(AtLeast(1));
  action_under_test->account_user_index_metadata =
      user_idx_md_factory->mock_account_user_index_metadata;
  action_under_test->get_next_buckets();
}

// Verify that GetNextBucketSuccessful list fetches correct count of bucket.
TEST_F(S3GetServiceActionTest, GetNextBucketSuccessful) {
  result_keys_values.insert(
      std::make_pair("testkey0", std::make_pair(0, "keyval")));
  result_keys_values.insert(
      std::make_pair("testkey1", std::make_pair(0, "keyval")));
  result_keys_values.insert(
      std::make_pair("testkey2", std::make_pair(0, "keyval")));

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_key_values())
      .WillRepeatedly(ReturnRef(result_keys_values));
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), from_json(_))
      .WillRepeatedly(Return(0));

  action_under_test->account_user_index_metadata =
      user_idx_md_factory->mock_account_user_index_metadata;
  action_under_test->get_next_buckets_successful();

  EXPECT_EQ(3, action_under_test->bucket_list.get_bucket_count());
}

// Verify that get_next_buckets_failed sets boolean flag to true if user
// index metadata is missing.i This calls send_response_to_s3_client
// with HTTP 200.
TEST_F(S3GetServiceActionTest, GetNextBucketFailedClovisReaderStateMissing) {
  // Set GTest expectations.
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::missing));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));

  // Perform action on test object.
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  action_under_test->get_next_buckets_failed();

  // Check state of the object.
  EXPECT_TRUE(action_under_test->fetch_successful);
}

// Verify that get_next_buckets_failed sets boolean flag to false if user
// index metadata is present. This calls send_response_to_s3_client
// with HTTP 500.
TEST_F(S3GetServiceActionTest, GetNextBucketFailedClovisReaderStatePresent) {
  // Set GTest expectations.
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::present));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  // Perform action on test object.
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  action_under_test->get_next_buckets_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  // Check state of the object.
  EXPECT_FALSE(action_under_test->fetch_successful);
}

// Verify fetch_bucket_list_idx_oid with mocked version of S3UserIdxMetadata.
TEST_F(S3GetServiceActionTest, FetchBucketListIndexOidWithMockedIdxMetadata) {
  EXPECT_CALL(*(user_idx_md_factory->mock_account_user_index_metadata),
              load(_, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid();
}

TEST_F(S3GetServiceActionTest, SendResponseToClientServiceUnavailable) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(AtLeast(1));
  action_under_test->check_shutdown_and_rollback();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3GetServiceActionTest, SendResponseToClientSuccess) {
  action_under_test->fetch_successful = true;
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

TEST_F(S3GetServiceActionTest, SendResponseToClientInternalError) {
  action_under_test->fetch_successful = false;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

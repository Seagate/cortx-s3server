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


#include "mero_uri.h"
#include "mock_mero_request_object.h"

using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;
using ::testing::ReturnRef;

class MeroURITEST : public testing::Test {
 protected:
  MeroURITEST() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockMeroRequestObject>(req, evhtp_obj_ptr);
  }

  std::shared_ptr<MockMeroRequestObject> ptr_mock_request;
};

class MeroPathStyleURITEST : public MeroURITEST {};

TEST_F(MeroURITEST, Constructor) {

  MeroURI merouri_test_obj(ptr_mock_request);
  EXPECT_STREQ("", merouri_test_obj.get_key_name().c_str());
  EXPECT_STREQ("", merouri_test_obj.get_object_oid_lo().c_str());
  EXPECT_STREQ("", merouri_test_obj.get_object_oid_hi().c_str());
  EXPECT_STREQ("", merouri_test_obj.get_index_id_lo().c_str());
  EXPECT_STREQ("", merouri_test_obj.get_index_id_hi().c_str());
  EXPECT_EQ(MeroApiType::unsupported, merouri_test_obj.get_mero_api_type());
  EXPECT_EQ(MeroOperationCode::none, merouri_test_obj.get_operation_code());
}

TEST_F(MeroPathStyleURITEST, KeyNameTest) {
  evhtp_request_t *req = NULL;
  EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
  std::shared_ptr<MockMeroRequestObject> ptr_mock_request =
      std::make_shared<MockMeroRequestObject>(req, evhtp_obj_ptr);

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/indexes/123-456/Object-key1"))
      .WillOnce(Return("/indexes/454-999/Object-key2"));

  MeroPathStyleURI meropathstyleone(ptr_mock_request);
  EXPECT_EQ(MeroApiType::keyval, meropathstyleone.get_mero_api_type());
  EXPECT_STREQ("Object-key1", meropathstyleone.get_key_name().c_str());
  EXPECT_STREQ("456", meropathstyleone.get_index_id_lo().c_str());
  EXPECT_STREQ("123", meropathstyleone.get_index_id_hi().c_str());

  MeroPathStyleURI meropathstyletwo(ptr_mock_request);
  EXPECT_EQ(MeroApiType::keyval, meropathstyletwo.get_mero_api_type());
  EXPECT_STREQ("Object-key2", meropathstyletwo.get_key_name().c_str());
  EXPECT_STREQ("999", meropathstyletwo.get_index_id_lo().c_str());
  EXPECT_STREQ("454", meropathstyletwo.get_index_id_hi().c_str());
}

TEST_F(MeroPathStyleURITEST, ObjectOidTest) {

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/objects/123-456"))
      .WillOnce(Return("/objects/454-999/"));

  MeroPathStyleURI meropathstyleone(ptr_mock_request);
  EXPECT_EQ(MeroApiType::object, meropathstyleone.get_mero_api_type());
  EXPECT_STREQ("456", meropathstyleone.get_object_oid_lo().c_str());
  EXPECT_STREQ("123", meropathstyleone.get_object_oid_hi().c_str());

  MeroPathStyleURI meropathstyletwo(ptr_mock_request);
  EXPECT_EQ(MeroApiType::object, meropathstyletwo.get_mero_api_type());
  EXPECT_STREQ("999", meropathstyletwo.get_object_oid_lo().c_str());
  EXPECT_STREQ("454", meropathstyletwo.get_object_oid_hi().c_str());
}

TEST_F(MeroPathStyleURITEST, IndexListTest) {

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/indexes/123-456"))
      .WillOnce(Return("/indexes/454-999/"));

  MeroPathStyleURI meropathstyleone(ptr_mock_request);
  EXPECT_EQ(MeroApiType::index, meropathstyleone.get_mero_api_type());
  EXPECT_STREQ("456", meropathstyleone.get_index_id_lo().c_str());
  EXPECT_STREQ("123", meropathstyleone.get_index_id_hi().c_str());

  MeroPathStyleURI meropathstyletwo(ptr_mock_request);
  EXPECT_EQ(MeroApiType::index, meropathstyletwo.get_mero_api_type());
  EXPECT_STREQ("999", meropathstyletwo.get_index_id_lo().c_str());
  EXPECT_STREQ("454", meropathstyletwo.get_index_id_hi().c_str());
}

TEST_F(MeroPathStyleURITEST, UnsupportedURITest) {

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/indexes123/123-456"))
      .WillOnce(Return("/objects123/"));

  MeroPathStyleURI meropathstyleone(ptr_mock_request);
  EXPECT_EQ(MeroApiType::unsupported, meropathstyleone.get_mero_api_type());

  MeroPathStyleURI meropathstyletwo(ptr_mock_request);
  EXPECT_EQ(MeroApiType::unsupported, meropathstyletwo.get_mero_api_type());
}
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

#include "motr_uri.h"
#include "mock_motr_request_object.h"

using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;
using ::testing::ReturnRef;

class MotrURITEST : public testing::Test {
 protected:
  MotrURITEST() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockMotrRequestObject>(req, evhtp_obj_ptr);
  }

  std::shared_ptr<MockMotrRequestObject> ptr_mock_request;
};

class MotrPathStyleURITEST : public MotrURITEST {};

TEST_F(MotrURITEST, Constructor) {

  MotrURI motruri_test_obj(ptr_mock_request);
  EXPECT_STREQ("", motruri_test_obj.get_key_name().c_str());
  EXPECT_STREQ("", motruri_test_obj.get_object_oid_lo().c_str());
  EXPECT_STREQ("", motruri_test_obj.get_object_oid_hi().c_str());
  EXPECT_STREQ("", motruri_test_obj.get_index_id_lo().c_str());
  EXPECT_STREQ("", motruri_test_obj.get_index_id_hi().c_str());
  EXPECT_EQ(MotrApiType::unsupported, motruri_test_obj.get_motr_api_type());
  EXPECT_EQ(MotrOperationCode::none, motruri_test_obj.get_operation_code());
}

TEST_F(MotrPathStyleURITEST, KeyNameTest) {
  evhtp_request_t *req = NULL;
  EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
  std::shared_ptr<MockMotrRequestObject> ptr_mock_request =
      std::make_shared<MockMotrRequestObject>(req, evhtp_obj_ptr);

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/indexes/123-456/Object-key1"))
      .WillOnce(Return("/indexes/454-999/Object-key2"));

  MotrPathStyleURI motrpathstyleone(ptr_mock_request);
  EXPECT_EQ(MotrApiType::keyval, motrpathstyleone.get_motr_api_type());
  EXPECT_STREQ("Object-key1", motrpathstyleone.get_key_name().c_str());
  EXPECT_STREQ("456", motrpathstyleone.get_index_id_lo().c_str());
  EXPECT_STREQ("123", motrpathstyleone.get_index_id_hi().c_str());

  MotrPathStyleURI motrpathstyletwo(ptr_mock_request);
  EXPECT_EQ(MotrApiType::keyval, motrpathstyletwo.get_motr_api_type());
  EXPECT_STREQ("Object-key2", motrpathstyletwo.get_key_name().c_str());
  EXPECT_STREQ("999", motrpathstyletwo.get_index_id_lo().c_str());
  EXPECT_STREQ("454", motrpathstyletwo.get_index_id_hi().c_str());
}

TEST_F(MotrPathStyleURITEST, ObjectOidTest) {

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/objects/123-456"))
      .WillOnce(Return("/objects/454-999/"));

  MotrPathStyleURI motrpathstyleone(ptr_mock_request);
  EXPECT_EQ(MotrApiType::object, motrpathstyleone.get_motr_api_type());
  EXPECT_STREQ("456", motrpathstyleone.get_object_oid_lo().c_str());
  EXPECT_STREQ("123", motrpathstyleone.get_object_oid_hi().c_str());

  MotrPathStyleURI motrpathstyletwo(ptr_mock_request);
  EXPECT_EQ(MotrApiType::object, motrpathstyletwo.get_motr_api_type());
  EXPECT_STREQ("999", motrpathstyletwo.get_object_oid_lo().c_str());
  EXPECT_STREQ("454", motrpathstyletwo.get_object_oid_hi().c_str());
}

TEST_F(MotrPathStyleURITEST, IndexListTest) {

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/indexes/123-456"))
      .WillOnce(Return("/indexes/454-999/"));

  MotrPathStyleURI motrpathstyleone(ptr_mock_request);
  EXPECT_EQ(MotrApiType::index, motrpathstyleone.get_motr_api_type());
  EXPECT_STREQ("456", motrpathstyleone.get_index_id_lo().c_str());
  EXPECT_STREQ("123", motrpathstyleone.get_index_id_hi().c_str());

  MotrPathStyleURI motrpathstyletwo(ptr_mock_request);
  EXPECT_EQ(MotrApiType::index, motrpathstyletwo.get_motr_api_type());
  EXPECT_STREQ("999", motrpathstyletwo.get_index_id_lo().c_str());
  EXPECT_STREQ("454", motrpathstyletwo.get_index_id_hi().c_str());
}

TEST_F(MotrPathStyleURITEST, UnsupportedURITest) {

  EXPECT_CALL(*ptr_mock_request, c_get_full_encoded_path())
      .WillOnce(Return("/indexes123/123-456"))
      .WillOnce(Return("/objects123/"));

  MotrPathStyleURI motrpathstyleone(ptr_mock_request);
  EXPECT_EQ(MotrApiType::unsupported, motrpathstyleone.get_motr_api_type());

  MotrPathStyleURI motrpathstyletwo(ptr_mock_request);
  EXPECT_EQ(MotrApiType::unsupported, motrpathstyletwo.get_motr_api_type());
}

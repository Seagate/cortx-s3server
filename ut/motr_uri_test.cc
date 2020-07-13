/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 09-July-2019
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
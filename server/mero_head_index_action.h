#ifndef __MERO_HEAD_INDEX_ACTION_H__
#define __MERO_HEAD_INDEX_ACTION_H__

#include <memory>
#include <gtest/gtest_prod.h>

#include "s3_factory.h"
#include "mero_action_base.h"

class MeroHeadIndexAction : public MeroAction {
  m0_uint128 index_id;
  std::shared_ptr<ClovisAPI> mero_clovis_api;
  std::shared_ptr<S3ClovisKVSReader> clovis_kv_reader;
  std::shared_ptr<S3ClovisKVSReaderFactory> clovis_kvs_reader_factory;

  void setup_steps();
  void validate_request();
  void check_index_exist();
  void check_index_exist_success();
  void check_index_exist_failure();
  void send_response_to_s3_client();

 public:
  MeroHeadIndexAction(
      std::shared_ptr<MeroRequestObject> req,
      std::shared_ptr<S3ClovisKVSReaderFactory> clovis_mero_kvs_reader_factory =
          nullptr);

  FRIEND_TEST(MeroHeadIndexActionTest, ValidIndexId);
  FRIEND_TEST(MeroHeadIndexActionTest, InvalidIndexId);
  FRIEND_TEST(MeroHeadIndexActionTest, EmptyIndexId);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExist);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistSuccess);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistFailureMissing);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistFailureInternalError);
  FRIEND_TEST(MeroHeadIndexActionTest, CheckIndexExistFailureFailedToLaunch);
  FRIEND_TEST(MeroHeadIndexActionTest, SendSuccessResponse);
  FRIEND_TEST(MeroHeadIndexActionTest, SendBadRequestResponse);
};
#endif

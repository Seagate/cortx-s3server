#pragma once

#ifndef __S3_SERVER_S3_HEAD_SERVICE_ACTION_H__
#define __S3_SERVER_S3_HEAD_SERVICE_ACTION_H__

#include <memory>
#include "s3_action_base.h"

class S3HeadServiceAction : public S3Action {
 public:
  S3HeadServiceAction(std::shared_ptr<S3RequestObject> req);
  void setup_steps();

  void send_response_to_s3_client();

  // google unit tests
  FRIEND_TEST(S3HeadServiceActionTest, SendResponseToClientServiceUnavailable);
};

#endif

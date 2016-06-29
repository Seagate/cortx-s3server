/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Abrarahmed Momin  <abrar.habib@seagate.com>
 * Original creation date: 9th-June-2016
 */

#include "gtest/gtest.h"
#include <functional>
#include <iostream>

#include "s3_action_base.h"
#include "s3_error_codes.h"
#include "mock_s3_request_object.h"
#include "mock_evhtp_wrapper.h"

using ::testing::Mock;
using ::testing::Return;
using ::testing::Eq;

// Before you create an object of this class, ensure
// auth is disabled for unit tests.
class S3ActionTestbase : public S3Action {
public:
  // Instantiate S3Action with authentication disabled
  S3ActionTestbase(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
    response_called = 0;
  };

  void send_response_to_s3_client() {
    response_called += 1;
  };

  // Declare viariables
  int response_called;
};

class S3ActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3ActionTest() {
    evbase_t *evbase = event_base_new();
    evhtp_request_t *req = evhtp_request_new(NULL, evbase);
    ptr_mock_request = std::make_shared<MockS3RequestObject> (req, new EvhtpWrapper());
    call_count_one = call_count_two = 0;
    S3Option::get_instance()->disable_auth();
    ptr_s3Actionobject = new S3ActionTestbase(ptr_mock_request);
  };

  ~S3ActionTest() {
    delete ptr_s3Actionobject;
  }

  // Declares the variables your tests want to use.
  std::function<void()> fptr_callback;
  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  S3ActionTestbase *ptr_s3Actionobject;

  int call_count_one;
  int call_count_two;

public:
  void func_callback_one() {
    call_count_one += 1;
  }
  void func_callback_two() {
    call_count_two += 1;
  }
};


TEST_F(S3ActionTest, Constructor) {
  EXPECT_EQ(NULL, ptr_s3Actionobject->task_iteration_index);
  EXPECT_TRUE(S3ActionState::start == ptr_s3Actionobject->state);
  EXPECT_EQ(0, ptr_s3Actionobject->task_list.size());
  EXPECT_EQ(0, ptr_s3Actionobject->rollback_list.size());
  EXPECT_EQ(NULL, call_count_one);
  EXPECT_EQ(NULL, call_count_two);
}


TEST_F(S3ActionTest, AddTask) {
  ptr_s3Actionobject->add_task(std::bind( &S3ActionTest::func_callback_one, this ));
  ptr_s3Actionobject->add_task(std::bind( &S3ActionTest::func_callback_two, this ));
  // test size of list
  EXPECT_EQ(2, ptr_s3Actionobject->task_list.size());
}

TEST_F(S3ActionTest, AddTaskRollback) {
  ptr_s3Actionobject->add_task_rollback(std::bind( &S3ActionTest::func_callback_one, this));
  ptr_s3Actionobject->add_task_rollback(std::bind( &S3ActionTest::func_callback_two, this));
  // test size of list
  EXPECT_EQ(2, ptr_s3Actionobject->rollback_list.size());
}

TEST_F(S3ActionTest, TasklistRun) {
  ptr_s3Actionobject->add_task(std::bind( &S3ActionTest::func_callback_one, this));
  ptr_s3Actionobject->add_task(std::bind( &S3ActionTest::func_callback_two, this));

  ptr_s3Actionobject->start();
  EXPECT_TRUE(call_count_one == 1);
  ptr_s3Actionobject->next();
  EXPECT_TRUE(call_count_two == 1);
}

TEST_F(S3ActionTest, RollbacklistRun) {
  ptr_s3Actionobject->add_task_rollback(std::bind( &S3ActionTest::func_callback_one, this));
  ptr_s3Actionobject->add_task_rollback(std::bind( &S3ActionTest::func_callback_two, this));

  EXPECT_EQ(2, ptr_s3Actionobject->rollback_list.size());

  ptr_s3Actionobject->rollback_start();
  EXPECT_TRUE(call_count_two == 1);
  ptr_s3Actionobject->rollback_next();
  EXPECT_TRUE(call_count_one == 1);
  ptr_s3Actionobject->rollback_next();
  EXPECT_EQ(1, ptr_s3Actionobject->response_called);
}

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "atexit.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class AtExitTest : public testing::Test {
 public:
  virtual void SetUp() {}
  void TearDown() {}
};

TEST_F(AtExitTest, BasicTest) {
  int call_count;

  call_count = 0;
  {  // Standard case.
    AtExit tst([&]() { call_count++; });
  }
  EXPECT_EQ(1, call_count);

  call_count = 0;
  {  // Cancel the call.
    AtExit tst([&]() { call_count++; });
    tst.cancel();
  }
  EXPECT_EQ(0, call_count);

  call_count = 0;
  {  // Cancel the call, then re-enable.
    AtExit tst([&]() { call_count++; });
    tst.cancel();
    tst.reenable();
  }
  EXPECT_EQ(1, call_count);

  call_count = 0;
  {  // Force deinit.
    AtExit tst([&]() { call_count++; });
    tst.call_now();
    EXPECT_EQ(1, call_count);
  }
  // make sure it's only called once
  EXPECT_EQ(1, call_count);

  call_count = 0;
  {  // Force deinit does not work when cancelled.
    AtExit tst([&]() { call_count++; });
    tst.cancel();
    tst.call_now();
    EXPECT_EQ(0, call_count);
  }
  // make sure it's not called in destructor
  EXPECT_EQ(0, call_count);
}

/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Vinayak Kale <vinayak.kale@seagate.com>
 * Original creation date: 31-October-2016
 */

#include "s3_stats.h"
#include "gtest/gtest.h"
#include "s3_option.h"

extern S3Option* g_option_instance;
extern S3Stats* g_stats_instance;

class S3StatsTest : public testing::Test {
 public:
  virtual void SetUp() {
    g_option_instance = S3Option::get_instance();
    g_option_instance->set_stats_enable(true);
    g_stats_instance = S3Stats::get_instance();
  }
};

TEST_F(S3StatsTest, Init) {
  ASSERT_TRUE(g_stats_instance != NULL);
  EXPECT_EQ(g_stats_instance->host, g_option_instance->get_statsd_ip_addr());
  EXPECT_EQ(g_stats_instance->port, g_option_instance->get_statsd_port());
  EXPECT_NE(g_stats_instance->sock, -1);
  EXPECT_FALSE(g_stats_instance->metrics_whitelist.empty());

  // test private utility functions
  EXPECT_TRUE(g_stats_instance->is_fequal(1.0, 1.0));
  EXPECT_FALSE(g_stats_instance->is_fequal(1.0, 1.01));
  EXPECT_FALSE(g_stats_instance->is_fequal(1.0, 0.99));

  EXPECT_TRUE(g_stats_instance->is_keyname_valid("bucket1"));
  EXPECT_TRUE(g_stats_instance->is_keyname_valid("bucket.object"));
  EXPECT_FALSE(g_stats_instance->is_keyname_valid("bucket@object"));
  EXPECT_FALSE(g_stats_instance->is_keyname_valid("bucket:object"));
  EXPECT_FALSE(g_stats_instance->is_keyname_valid("bucket|object"));
  EXPECT_FALSE(g_stats_instance->is_keyname_valid("@bucket|object:"));
}

TEST_F(S3StatsTest, Whitelist) {
  EXPECT_TRUE(g_stats_instance->is_allowed_to_publish("uri_to_mero_oid"));
  EXPECT_TRUE(g_stats_instance->is_allowed_to_publish(
      "delete_object_from_clovis_failed"));
  EXPECT_TRUE(g_stats_instance->is_allowed_to_publish("total_request_time"));
  EXPECT_TRUE(g_stats_instance->is_allowed_to_publish(
      "get_bucket_location_request_count"));
  EXPECT_TRUE(
      g_stats_instance->is_allowed_to_publish("get_object_acl_request_count"));
  EXPECT_TRUE(
      g_stats_instance->is_allowed_to_publish("get_service_request_count"));
  EXPECT_FALSE(g_stats_instance->is_allowed_to_publish("xyz"));

  // test loading of non-existing whitelist yaml file
  g_option_instance->set_stats_whitelist_filename(
      "non-existing-whitelist.yaml");
  EXPECT_NE(g_stats_instance->load_whitelist(), 0);
  g_option_instance->set_stats_whitelist_filename(
      "s3stats-whitelist-test.yaml");
}

// TODO (Vinayak)
// Mock `send` method and test public member functions.

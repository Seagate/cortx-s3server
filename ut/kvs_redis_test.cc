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
 * Original author:  Dmitrii Surnin <dmitrii.surnin@seagate.com>
 * Original creation date: 19-September-2019
 */

#include "s3_test_utils.h"
#include "s3_fake_clovis_redis_kvs_internal.h"

TEST(RedisKvs, parse_key) {
  char kv[] = {'k', 'e', 'y', 0, 'v', 'a', 'l', 0};
  char* k = parse_key(kv, 8);
  EXPECT_EQ(strcmp(k, "key"), 0);

  k = parse_key(nullptr, 0);
  EXPECT_EQ(k, nullptr);

  std::string key_only("key_1");
  k = parse_key(const_cast<char*>(key_only.c_str()), key_only.length());
  EXPECT_EQ(strcmp(k, "key_1"), 0);
}

TEST(RedisKvs, parse_val) {
  char kv[] = {'k', 'e', 'y', 0, 'v', 'a', 'l', 0};
  char* v = parse_val(kv, 8);
  EXPECT_EQ(strcmp(v, "val"), 0);

  v = parse_val(nullptr, 0);
  EXPECT_EQ(v, nullptr);

  std::string key_only("key_1");
  v = parse_val(const_cast<char*>(key_only.c_str()), key_only.length());
  EXPECT_EQ(v, nullptr);
}

TEST(RedisKvs, prepare_rkey) {
  std::string k("key");
  std::string v("val");
  redis_key rk = prepare_rkey(k.c_str(), k.length(), v.c_str(), v.length());

  ASSERT_NE(rk.buf, nullptr);
  EXPECT_EQ(rk.len, k.length() + v.length() + 2);
  EXPECT_EQ(strcmp(parse_key(rk.buf, rk.len), k.c_str()), 0);
  EXPECT_EQ(strcmp(parse_val(rk.buf, rk.len), v.c_str()), 0);

  free(rk.buf);
}

TEST(RedisKvs, prepare_border) {
  std::string b("border");
  redis_key brdr = prepare_border(b.c_str(), b.length(), true, false);

  ASSERT_NE(brdr.buf, nullptr);
  EXPECT_EQ(brdr.len, b.length() + 1);
  EXPECT_EQ(strcmp(brdr.buf, ("[" + b).c_str()), 0);

  free(brdr.buf);
  brdr = {0, nullptr};

  brdr = prepare_border(b.c_str(), b.length(), false, false);

  ASSERT_NE(brdr.buf, nullptr);
  EXPECT_EQ(brdr.len, b.length() + 1);
  EXPECT_EQ(strcmp(brdr.buf, ("(" + b).c_str()), 0);

  free(brdr.buf);
  brdr = {0, nullptr};

  brdr = prepare_border(b.c_str(), b.length(), true, true);

  ASSERT_NE(brdr.buf, nullptr);
  EXPECT_EQ(brdr.len, b.length() + 2);
  EXPECT_EQ(strcmp(brdr.buf, ("[" + b + '\xff').c_str()), 0);

  free(brdr.buf);
  brdr = {0, nullptr};

  brdr = prepare_border(b.c_str(), b.length(), false, true);

  ASSERT_NE(brdr.buf, nullptr);
  EXPECT_EQ(brdr.len, b.length() + 2);
  EXPECT_EQ(strcmp(brdr.buf, ("(" + b + '\xff').c_str()), 0);

  free(brdr.buf);
  brdr = {0, nullptr};
}

TEST(RedisKvs, redis_reply_check) {
  struct m0_clovis_op op;
  s3_redis_context_obj rco;
  op.op_datum = &rco;

  redisAsyncContext rac;
  redisReply rr;
  s3_redis_async_ctx priv;

  EXPECT_EQ(redis_reply_check(nullptr, nullptr, nullptr, {}), REPL_ERR);
  EXPECT_EQ(redis_reply_check(&rac, &rr, nullptr, {}), REPL_ERR);

  priv.op = &op;
  rco.had_error = false;

  EXPECT_EQ(redis_reply_check(nullptr, nullptr, &priv, {}), REPL_DONE);
  EXPECT_EQ(rco.had_error, true);

  rco.had_error = false;
  rac.err = -ENOENT;
  EXPECT_EQ(redis_reply_check(&rac, &rr, &priv, {}), REPL_DONE);
  EXPECT_EQ(rco.had_error, true);

  rco.had_error = false;
  rac.err = 0;
  rr.type = 3;
  EXPECT_EQ(redis_reply_check(&rac, &rr, &priv, {1, 2, 4, 5}), REPL_DONE);
  EXPECT_EQ(rco.had_error, true);

  rco.had_error = false;
  rac.err = 0;
  rr.type = 3;
  EXPECT_EQ(redis_reply_check(&rac, &rr, &priv, {1, 2, 3, 5}), REPL_CONTINUE);
  EXPECT_EQ(rco.had_error, false);
}

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

package com.seagates3.policy;

import org.junit.Assert;
import org.junit.Test;

public
class ConditionUtilTest {

  ConditionUtil util = ConditionUtil.getInstance();

  /**
   * Validate StringEquals valid condition
   */
  @Test public void testIsConditionTypeValid_StringEquals_success() {
    Assert.assertTrue(util.isConditionTypeValid("StringEquals"));
  }

  /**
   * Validate StringEqualsIfExists valid condition
   */
  @Test public void testIsConditionTypeValid_StringEqualsIfExists_success() {
    Assert.assertTrue(util.isConditionTypeValid("StringEquals"));
  }

  /**
   * Validate Bool valid condition
   */
  @Test public void testIsConditionTypeValid_Bool_success() {
    Assert.assertTrue(util.isConditionTypeValid("Bool"));
  }

  /**
   * Validate condition
   */
  @Test public void testIsConditionTypeValid_empty_fail() {
    Assert.assertFalse(util.isConditionTypeValid(""));
  }

  /**
   * Validate condition
   */
  @Test public void testIsConditionTypeValid_NullType_success() {
    Assert.assertTrue(util.isConditionTypeValid("Null"));
  }

  /**
   * Validate condition
   */
  @Test public void testIsConditionTypeValid_null_fail() {
    Assert.assertFalse(util.isConditionTypeValid(null));
  }

  /**
   * Validate condition
   */
  @Test public void testIsConditionTypeValid_randomValue_fail() {
    Assert.assertFalse(util.isConditionTypeValid("qwerty"));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_invalid_SourceArn_success() {
    Assert.assertFalse(util.isConditionKeyValid("CurrentTime"));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_awsRandom_success() {
    Assert.assertTrue(util.isConditionKeyValid("aws:qwerty"));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_awsEmpty_success() {
    Assert.assertTrue(util.isConditionKeyValid("aws:"));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_s3_success() {
    Assert.assertTrue(util.isConditionKeyValid("s3:x-amz-acl"));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_s3_fail() {
    Assert.assertFalse(util.isConditionKeyValid("s3:xyz"));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_null_fail() {
    Assert.assertFalse(util.isConditionKeyValid(null));
  }

  /**
   * Validate Condition Key
   */
  @Test public void testIsConditionKeyValid_empty_fail() {
    Assert.assertFalse(util.isConditionKeyValid(""));
  }

  /**
   * Validate removeKeyPrefix
   */
  @Test public void testRemoveKeyPrefix_s3_success() {
    Assert.assertEquals("abc", ConditionUtil.removeKeyPrefix("s3:abc"));
  }

  /**
   * Validate removeKeyPrefix
   */
  @Test public void testRemoveKeyPrefix_aws_success() {
    Assert.assertEquals("abc", ConditionUtil.removeKeyPrefix("aws:abc"));
  }

  /**
   * Validate removeKeyPrefix
   */
  @Test public void testRemoveKeyPrefix_fail() {
    Assert.assertEquals("abc:abc", ConditionUtil.removeKeyPrefix("abc:abc"));
  }

  /**
   * Validate removeKeyPrefix
   */
  @Test public void testRemoveKeyPrefix_fail2() {
    Assert.assertEquals("abc", ConditionUtil.removeKeyPrefix("abc"));
  }

  /**
   * Validate removeKeyPrefix
   */
  @Test public void testRemoveKeyPrefix_empty() {
    Assert.assertEquals("", ConditionUtil.removeKeyPrefix(""));
  }

  /**
   * Validate removeKeyPrefix
   */
  @Test public void testRemoveKeyPrefix_null() {
    Assert.assertNull(ConditionUtil.removeKeyPrefix(null));
  }

  /**
   * Validate Condition combination
   */
  @Test public void isConditionCombinationValid_true() {
    Assert.assertTrue(ConditionUtil.getInstance().isConditionCombinationValid(
        "s3:x-amz-acl", "s3:PutObject"));
  }

  /**
   * Validate Condition combination
   */
  @Test public void isConditionCombinationValid_false() {
    Assert.assertFalse(ConditionUtil.getInstance().isConditionCombinationValid(
        "s3:x-amz", "s3:PutObject"));
  }

  /**
   * Validate Condition combination
   */
  @Test public void isConditionCombinationValid_nullKey() {
    Assert.assertFalse(ConditionUtil.getInstance().isConditionCombinationValid(
        null, "s3:PutObject"));
  }

  /**
   * Validate Condition combination
   */
  @Test public void isConditionCombinationValid_nullVal() {
    Assert.assertFalse(ConditionUtil.getInstance().isConditionCombinationValid(
        "s3-x-amz-acl", null));
  }

  /**
   * Validate Condition combination
   */
  @Test public void isConditionCombinationValid_caseInsensitive() {
    Assert.assertTrue(ConditionUtil.getInstance().isConditionCombinationValid(
        "s3:X-AMZ-acl", "s3:PutObject"));
  }
}

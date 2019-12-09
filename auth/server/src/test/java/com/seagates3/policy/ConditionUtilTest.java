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
 * Original author:  Ajinkya Dhumal
 * Original creation date: 29-November-2019
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
}

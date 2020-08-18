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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.junit.Before;
import org.junit.Test;

import com.seagates3.policy.BooleanCondition.BooleanComparisonType;

public
class BoolConditionTest {

  BooleanCondition condition = null;
  static List<String> values = null;
  Map<String, String> requestBody = null;
  String key = "SecureTransport";
  String invalidKeyHeader = "x-amz";
  String invalidValue = "invalid-value";

  @Before public void setUp() throws Exception {
    values = new ArrayList<>();
    requestBody = new HashMap<String, String>();
  }

  @Test public void testIsSatisfied_Bool_true_success() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "true");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_True_success() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "True");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_false_success() {
    values.add("false");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "false");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_False_success() {
    values.add("False");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "fAlse");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_trueAndfalse_success() {
    values.add("false");
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "false");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_trueAndfalse_success2() {
    values.add("false");
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "true");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_fail() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "false");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_fail2() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "abcd");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_fail3() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, "");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_fail4() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(key, null);
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_fail5() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, values);
    requestBody.put(invalidKeyHeader, "true");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_nullKey_fail() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, null, values);
    requestBody.put(key, "true");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_Bool_true_nullValues_fail() {
    values.add("true");
    condition = new BooleanCondition(BooleanComparisonType.Bool, key, null);
    requestBody.put(key, "true");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_BoolIfExists_true_success() {
    values.add("true");
    condition =
        new BooleanCondition(BooleanComparisonType.BoolIfExists, key, values);
    requestBody.put(invalidKeyHeader, "true");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_BoolIfExists_true_success2() {
    values.add("true");
    condition =
        new BooleanCondition(BooleanComparisonType.BoolIfExists, key, values);
    requestBody.put(key, "true");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_BoolIfExists_true_fail() {
    values.add("true");
    condition =
        new BooleanCondition(BooleanComparisonType.BoolIfExists, key, values);
    requestBody.put(key, "false");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_BoolIfExists_false_success() {
    values.add("false");
    condition =
        new BooleanCondition(BooleanComparisonType.BoolIfExists, key, values);
    requestBody.put(invalidKeyHeader, "true");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_BoolIfExists_false_fail() {
    values.add("false");
    condition =
        new BooleanCondition(BooleanComparisonType.BoolIfExists, key, values);
    requestBody.put(key, "true");
    assertFalse(condition.isSatisfied(requestBody));
  }
}

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

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.junit.Before;
import org.junit.Test;

public
class NullConditionTest {

  NullCondition condition = null;
  List<String> values = null;
  Map<String, String> requestBody = null;
  String key = "x-amz-acl";
  String invalidKeyHeader = "x-amz";

  @Before public void setUp() throws Exception {
    values = new ArrayList<>();
    requestBody = new HashMap<String, String>();
  }

  @Test public void testIsSatisfied_true_success() {
    values.add("true");
    condition = new NullCondition(key, values);
    requestBody.put(invalidKeyHeader, "abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_true_fail() {
    values.add("true");
    condition = new NullCondition(key, values);
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_true_keyNull_success() {
    values.add("true");
    condition = new NullCondition(null, values);
    requestBody.put(key, "abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_false_keyNull_fail() {
    values.add("false");
    condition = new NullCondition(null, values);
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_true_valueNull_success() {
    values.add("true");
    condition = new NullCondition(key, null);
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_false_success() {
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(key, "abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_false_success2() {
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(key, "");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_false_fail() {
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(key, null);
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_false_fail2() {
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(invalidKeyHeader, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_trueAndfalse_success() {
    values.add("true");
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(key, "abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_trueAndfalse_success3() {
    values.add("true");
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(invalidKeyHeader, "abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_trueAndfalse_fail() {
    values.add("true");
    values.add("false");
    condition = new NullCondition(key, values);
    requestBody.put(key, null);
    assertFalse(condition.isSatisfied(requestBody));
  }
}

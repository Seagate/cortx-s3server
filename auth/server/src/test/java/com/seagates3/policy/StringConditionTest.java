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

import org.junit.BeforeClass;
import org.junit.Test;

import com.seagates3.policy.StringCondition.StringComparisonType;

public
class StringConditionTest {

  StringCondition condition = null;
  static List<String> values = new ArrayList<>();
  Map<String, String> requestBody = null;
  String key = "x-amz-acl";
  String invalidKeyHeader = "x-amz";
  String invalidValue = "invalid-value";

  @BeforeClass public static void setUpBeforeClass() {
    values.add("abc");
    values.add("Pqr");
    values.add("qwe*");
    values.add("");
  }

  @Test public void testIsSatisfied_stringEquals_success() {
    condition =
        new StringCondition(StringComparisonType.StringEquals, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEquals_fail() {
    condition =
        new StringCondition(StringComparisonType.StringEquals, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "Abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEquals_success() {
    condition =
        new StringCondition(StringComparisonType.StringNotEquals, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "Abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEquals_fail() {
    condition =
        new StringCondition(StringComparisonType.StringNotEquals, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIgnoreCase_success() {
    condition = new StringCondition(StringComparisonType.StringEqualsIgnoreCase,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "Abc");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIgnoreCase_fail() {
    condition = new StringCondition(StringComparisonType.StringEqualsIgnoreCase,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "xyz");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEqualsIgnoreCase_success() {
    condition = new StringCondition(
        StringComparisonType.StringNotEqualsIgnoreCase, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "xyz");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEqualsIgnoreCase_fail() {
    condition = new StringCondition(
        StringComparisonType.StringNotEqualsIgnoreCase, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "Abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringLike_success() {
    condition =
        new StringCondition(StringComparisonType.StringLike, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "qwerty");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringLike_fail() {
    condition =
        new StringCondition(StringComparisonType.StringLike, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "bqwer");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotLike_success() {
    condition =
        new StringCondition(StringComparisonType.StringNotLike, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "bqwer");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotLike_fail() {
    condition =
        new StringCondition(StringComparisonType.StringNotLike, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "qwer");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIfExists_invalidKey_success() {
    condition = new StringCondition(StringComparisonType.StringEqualsIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(invalidKeyHeader, invalidValue);
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIfExists_validKey_success() {
    condition = new StringCondition(StringComparisonType.StringEqualsIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "Pqr");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIfExists_fail() {
    condition = new StringCondition(StringComparisonType.StringEqualsIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, invalidValue);
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEqualsIfExists_success() {
    condition = new StringCondition(
        StringComparisonType.StringNotEqualsIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(invalidKeyHeader, invalidValue);
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEqualsIfExists_fail() {
    condition = new StringCondition(
        StringComparisonType.StringNotEqualsIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIgnoreCaseIfExists_success1() {
    condition = new StringCondition(
        StringComparisonType.StringEqualsIgnoreCaseIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "pqr");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIgnoreCaseIfExists_success2() {
    condition = new StringCondition(
        StringComparisonType.StringEqualsIgnoreCaseIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(invalidKeyHeader, "pqrst");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEqualsIgnoreCaseIfExists_fail() {
    condition = new StringCondition(
        StringComparisonType.StringEqualsIgnoreCaseIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "pqrst");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void
  testIsSatisfied_stringNotEqualsIgnoreCaseIfExists_success() {
    condition = new StringCondition(
        StringComparisonType.StringNotEqualsIgnoreCaseIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(invalidKeyHeader, "pqrst");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotEqualsIgnoreCaseIfExists_fail() {
    condition = new StringCondition(
        StringComparisonType.StringNotEqualsIgnoreCaseIfExists, key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "pqr");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringLikeIfExists_success() {
    condition = new StringCondition(StringComparisonType.StringLikeIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(invalidKeyHeader, "pqrst");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringLikeIfExists_fail() {
    condition = new StringCondition(StringComparisonType.StringLikeIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "pqrst");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotLikeIfExists_success1() {
    condition = new StringCondition(StringComparisonType.StringNotLikeIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(invalidKeyHeader, "pqr");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotLikeIfExists_success2() {
    condition = new StringCondition(StringComparisonType.StringNotLikeIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "pqrst");
    assertTrue(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringNotLikeIfExists_fail() {
    condition = new StringCondition(StringComparisonType.StringNotLikeIfExists,
                                    key, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "Pqr");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEquals_keyNull_fail() {
    condition =
        new StringCondition(StringComparisonType.StringEquals, null, values);
    requestBody = new HashMap<>();
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }

  @Test public void testIsSatisfied_stringEquals_valuesNull_fail() {
    condition =
        new StringCondition(StringComparisonType.StringEquals, key, null);
    requestBody = new HashMap<>();
    requestBody.put(key, "abc");
    assertFalse(condition.isSatisfied(requestBody));
  }
}

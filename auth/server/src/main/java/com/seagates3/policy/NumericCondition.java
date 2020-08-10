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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

public
class NumericCondition extends PolicyCondition {

 public
  static enum NumericComparisonType {
    NumericEquals,
    NumericGreaterThan,
    NumericGreaterThanEquals,
    NumericLessThan,
    NumericLessThanEquals,
    NumericNotEquals,
    NumericEqualsIfExists,
    NumericGreaterThanIfExists,
    NumericGreaterThanEqualsIfExists,
    NumericLessThanIfExists,
    NumericLessThanEqualsIfExists,
    NumericNotEqualsIfExists;
  };

 public NumericCondition(NumericComparisonType type, String key,
                         List<String> values) {
    if (type == null)
      super.type = null;
    else
      super.type = type.toString();
    super.conditionKey = key;
    super.values = values;
  }

  /**
   * Checks if this Numeric Condition is satisfied from the request.
   * Sample condition -
   * "Condition": {"NumericLessThanEquals": {"s3:max-keys": "10"}}
   */
  @Override public boolean isSatisfied(Map<String, String> requestBody) {
    if (this.values == null) return false;
    NumericComparisonType enumType = NumericComparisonType.valueOf(this.type);
    if (enumType == null) return false;

    // Fetch the header value for corresponding Condition key
    String headerVal = null;
    for (Entry<String, String> entry : requestBody.entrySet()) {
      if (entry.getKey().equalsIgnoreCase(this.conditionKey)) {
        headerVal = entry.getValue();
      } else if (entry.getKey().equals("ClientQueryParams")) {
        headerVal = PolicyUtil.fetchQueryParamValue(entry.getValue(),
                                                    this.conditionKey);
        if (headerVal != null) break;
      }
    }
    boolean result = false;

    switch (enumType) {

      case NumericEquals:
        result = numericEquals(headerVal);
        break;

      case NumericGreaterThan:
        result = numericGreaterThan(headerVal);
        break;

      case NumericGreaterThanEquals:
        result = numericGreaterThanEquals(headerVal);
        break;

      case NumericLessThan:
        if (headerVal != null) result = !numericGreaterThanEquals(headerVal);
        break;

      case NumericLessThanEquals:
        if (headerVal != null) result = !numericGreaterThan(headerVal);
        break;

      case NumericNotEquals:
        result = !numericEquals(headerVal);
        break;

      case NumericEqualsIfExists:
        if (headerVal != null)
          result = numericEquals(headerVal);
        else
          result = true;
        break;

      case NumericGreaterThanIfExists:
        if (headerVal != null)
          result = numericGreaterThan(headerVal);
        else
          result = true;
        break;

      case NumericGreaterThanEqualsIfExists:
        if (headerVal != null)
          result = numericGreaterThanEquals(headerVal);
        else
          result = true;
        break;

      case NumericLessThanIfExists:
        if (headerVal != null)
          result = !numericGreaterThanEquals(headerVal);
        else
          result = true;
        break;

      case NumericLessThanEqualsIfExists:
        if (headerVal != null)
          result = !numericGreaterThan(headerVal);
        else
          result = true;
        break;

      case NumericNotEqualsIfExists:
        if (headerVal != null)
          result = !numericEquals(headerVal);
        else
          result = true;
        break;

      default:
        result = false;
        break;
    }
    return result;
  }

  /**
   * Check if the input String is equal to any of the condition values.
   * Returns false if the headerVal or any of the condition values is not a
   * {@link Number}
   * @param headerVal
   * @return
   */
 private
  boolean numericEquals(String headerVal) {
    if (headerVal == null) return false;
    Long longHeader = null;
    List<Long> longValues = null;
    try {
      longHeader = Long.parseLong(headerVal.trim());
      longValues = new ArrayList<Long>();
      for (String s : this.values) {
        longValues.add(Long.parseLong(s));
      }
    }
    catch (Exception ex) {
      return false;
    }
    return longValues.contains(longHeader);
  }

  /**
   * Check if the input String is greater than any of the condition values.
   * Returns false if the headerVal or any of the condition values is not a
   * {@link Number}
   * @param headerVal
   * @return
   */
 private
  boolean numericGreaterThan(String headerVal) {
    if (headerVal == null) return false;
    Long longHeader = null;
    try {
      longHeader = Long.parseLong(headerVal.trim());
      for (String s : this.values) {
        Long conditionVal = Long.parseLong(s);
        if (longHeader > conditionVal) return true;
      }
    }
    catch (Exception ex) {
      return false;
    }
    return false;
  }

  /**
   * Check if the input String is greater than or equals to any of the condition
   * values. Returns false if the headerVal or any of the condition values is
   * not
   * a {@link Number}
   * @param headerVal
   * @return
   */
 private
  boolean numericGreaterThanEquals(String headerVal) {
    if (headerVal == null) return false;
    Long longHeader = null;
    try {
      longHeader = Long.parseLong(headerVal.trim());
      for (String s : this.values) {
        Long conditionVal = Long.parseLong(s);
        if (longHeader >= conditionVal) return true;
      }
    }
    catch (Exception ex) {
      return false;
    }
    return false;
  }

  /**
   * Returns the enum value of the String
   * @param enumName
   * @return
   */
 public
  static NumericComparisonType getEnum(String enumName) {
    try {
      return NumericComparisonType.valueOf(enumName);
    }
    catch (Exception ex) {
      return null;
    }
  }
}

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
class BooleanCondition extends PolicyCondition {

 public
  static enum BooleanComparisonType {
    Bool,
    BoolIfExists;
  };

 public BooleanCondition(BooleanComparisonType type, String key,
                         List<String> values) {
    if (type == null)
      super.type = null;
    else
      super.type = type.toString();
    super.conditionKey = key;
    super.values = values;
  }

  /**
   * Checks if this Boolean Condition is satisfied from the request.
   * Sample condition -
   * "Condition": {"Bool": {"aws:SecureTransport": "true"}}
   */
  @Override public boolean isSatisfied(Map<String, String> requestBody) {
    if (this.values == null) return false;
    BooleanComparisonType enumType = BooleanComparisonType.valueOf(this.type);
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
      case Bool:
        result = booleanEquals(headerVal);
        break;

      case BoolIfExists:
        if (headerVal != null)
          result = booleanEquals(headerVal);
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
  boolean booleanEquals(String headerVal) {
    if (headerVal == null) return false;
    Boolean boolHeader = null;
    List<Boolean> boolValues = null;
    boolHeader = Boolean.parseBoolean(headerVal.trim());
    boolValues = new ArrayList<Boolean>();
    for (String s : this.values) {
      if (!("true".equalsIgnoreCase(s) || "false".equalsIgnoreCase(s)))
        continue;
      boolValues.add(Boolean.parseBoolean(s));
    }
    return boolValues.contains(boolHeader);
  }

  /**
   * Returns the enum value of the String
   * @param enumName
   * @return
   */
 public
  static BooleanComparisonType getEnum(String enumName) {
    try {
      return BooleanComparisonType.valueOf(enumName);
    }
    catch (Exception ex) {
      return null;
    }
  }
}

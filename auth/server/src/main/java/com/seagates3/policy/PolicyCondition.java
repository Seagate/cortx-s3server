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

import java.util.List;
import java.util.Map;

public
class PolicyCondition {

 protected
  String type;
 protected
  String conditionKey;
 protected
  List<String> values;

  /**
   * Returns the type of this condition.
   *
   * @return the type of this condition.
   */
 public
  String getType() { return type; }

  /**
   * Sets the type of this condition.
   *
   * @param type
   *            the type of this condition.
   */
 public
  void setType(String type) { this.type = type; }

  /**
   * @return The name of the condition key involved in this condition.
   */
 public
  String getConditionKey() { return conditionKey; }

  /**
   * @param conditionKey
   *            The name of the condition key involved in this condition.
   */
 public
  void setConditionKey(String conditionKey) {
    this.conditionKey = conditionKey;
  }

  /**
   * @return The values specified for this access control policy condition.
   */
 public
  List<String> getValues() { return values; }

  /**
   * @param values
   *            The values specified for this access control policy condition.
   */
 public
  void setValues(List<String> values) { this.values = values; }

  /**
   * Checks if the condition is satisfied from the request
   * @param requestBody
   * @return
   */
 public
  boolean isSatisfied(Map<String, String> requestBody) {
    // TODO: Add logic to check the headers as per generic Condition type
    // OR keep abstract
    return false;
  }
}

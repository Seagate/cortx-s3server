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

public
class ConditionFactory {

  /**
   * Return the Condition class depending upon the ConditionType
   * @param ConditionType
   * @param key
   * @param values
   * @return
   */
 public
  static PolicyCondition getCondition(String ConditionType, String key,
                                      List<String> values) {

    if (StringCondition.getEnum(ConditionType) != null) {
      return new StringCondition(StringCondition.getEnum(ConditionType), key,
                                 values);
    } else if (DateCondition.getEnum(ConditionType) != null) {
      return new DateCondition(DateCondition.getEnum(ConditionType), key,
                               values);
    } else if (NumericCondition.getEnum(ConditionType) != null) {
      return new NumericCondition(NumericCondition.getEnum(ConditionType), key,
                                  values);
    } else if (BooleanCondition.getEnum(ConditionType) != null) {
      return new BooleanCondition(BooleanCondition.getEnum(ConditionType), key,
                                  values);
    } else if ("Null".equals(ConditionType)) {
      return new NullCondition(key, values);
    } else {
      return null;
    }
  }
}


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

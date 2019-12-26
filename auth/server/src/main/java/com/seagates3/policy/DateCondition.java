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
 * Original creation date: 26-December-2019
 */

package com.seagates3.policy;

import java.util.List;
import java.util.Map;

public
class DateCondition extends PolicyCondition {

 public
  static enum DateComparisonType {
    DateEquals,
    DateGreaterThan,
    DateGreaterThanEquals,
    DateLessThan,
    DateLessThanEquals,
    DateNotEquals,
    DateEqualsIfExists,
    DateGreaterThanIfExists,
    DateGreaterThanEqualsIfExists,
    DateLessThanIfExists,
    DateLessThanEqualsIfExists,
    DateNotEqualsIfExists;
  };

 public DateCondition(DateComparisonType type, String key,
                      List<String> values) {
    if (type == null)
      super.type = null;
    else
      super.type = type.toString();
    super.conditionKey = key;
    super.values = values;
  }

  /**
   * Checks if this Date Condition is satisfied from the request.
   * Sample condition -
   * "Condition":
   *        {"DateGreaterThan": {"aws:TokenIssueTime": "2020-01-01T00:00:01Z"}}
   */
  @Override public boolean isSatisfied(Map<String, String> requestBody) {
    // TODO: Add logic to check the headers as per Date Condition type
    return true;
  }
}

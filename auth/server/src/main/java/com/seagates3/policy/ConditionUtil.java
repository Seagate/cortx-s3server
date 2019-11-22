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

import java.io.InputStreamReader;
import java.lang.reflect.Type;
import java.util.HashSet;
import java.util.List;

import org.apache.commons.codec.binary.Base64;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.reflect.TypeToken;

public
class ConditionUtil {

 private
  HashSet<String> conditionTypes = new HashSet<>();

 private
  HashSet<String> conditionKeys = new HashSet<>();

 private
  static final String CONDITION_TYPES_FILE = "/policy/ConditionTypes.json";

 private
  static final String CONDITION_KEYS_FILE = "/policy/ConditionKeys.json";

  /**
   * private constructor - initializes the ConditionTypes and ConditionKeys
   */
 private
  ConditionUtil() {
    initConditionTypes();
    initConditionKeys();
  }

  /**
   * Singleton class holder
   */
 private
  static class ConditionUtilHolder {
    static final ConditionUtil CONDITION_INSTANCE = new ConditionUtil();
  }

  /**
   * Gets the singleton instance of {@link ConditionUtil}
   * @return {@link ConditionUtil} instance
   */
  public static ConditionUtil
  getInstance() {
    return ConditionUtilHolder.CONDITION_INSTANCE;
  }

  /**
   * Initialize conditionTypes set with the condition types from
   * ConditionTypes.json file
   */
 private
  void initConditionTypes() {
    InputStreamReader reader = new InputStreamReader(
        PolicyValidator.class.getResourceAsStream(CONDITION_TYPES_FILE));

    Type setType = new TypeToken<HashSet<String>>() {}
    .getType();
    JsonParser jsonParser = new JsonParser();
    JsonObject element = (JsonObject)jsonParser.parse(reader);
    conditionTypes =
        new Gson().fromJson(element.get("ConditionTypes"), setType);
  }

  /**
   * Initialize conditionKeys set with the condition types from
   * ConditionKeys.json file
   */
 private
  void initConditionKeys() {
    InputStreamReader reader = new InputStreamReader(
        PolicyValidator.class.getResourceAsStream(CONDITION_KEYS_FILE));

    Gson gson = new Gson();
    Type setType = new TypeToken<HashSet<String>>() {}
    .getType();
    JsonParser jsonParser = new JsonParser();
    JsonObject element = (JsonObject)jsonParser.parse(reader);
    HashSet<String> globalKeys =
        gson.fromJson(element.get("GlobalKeys"), setType);
    HashSet<String> s3Keys = gson.fromJson(element.get("S3Keys"), setType);
    conditionKeys.addAll(globalKeys);
    conditionKeys.addAll(s3Keys);
  }

  /**
   * Check if the condition type is valid
   * Condition types can be subtypes of - String, Numeric, Date, Boolean,
   * Arn, Binary, IpAddress, ..IfExists, Null
   * @param condition
   * @return
   */
 public
  boolean isConditionTypeValid(String conditionType) {
    if (conditionTypes.contains(conditionType)) {
      return true;
    }
    return false;
  }

  /**
   * Check if the condition key is valid
   * The condition keys could be one of - AWS global keys / custom keys
   * or S3 specific keys
   * e.g. - AWS wide keys - aws:CurrentTime, aws:SourceIp, aws:SourceArn, etc.
   * S3 specific keys - s3:x-amz-acl, s3:x-amz-copy-source, etc.
   * Custom Keys can also be used which should be in format - aws:<key>
   * @param condition
   * @return - true if the Condition key is one of the above specified keys
   */
 public
  boolean isConditionKeyValid(String conditionKey) {
    if (conditionKeys.contains(conditionKey) ||
        conditionKey != null && conditionKey.startsWith("aws:")) {
      return true;
    }
    return false;
  }

  /**
   * Check if the condition value is valid.
   * Only value for Binary condition is validated here.
   * For everything else return true.
   * @param condition
   * @return
   */
 public
  boolean isConditionValueValid(String conditionType, List<String> values) {
    if (conditionType != null &&
        (conditionType.equals("BinaryEquals") ||
         conditionType.equals("BinaryEqualsIfExists"))) {

      if (values == null || values.isEmpty() ||
          !Base64.isBase64(values.get(0))) {
        return false;
      }
    }
    return true;
  }
}

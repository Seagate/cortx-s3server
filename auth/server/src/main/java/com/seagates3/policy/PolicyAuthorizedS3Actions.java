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

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public
class PolicyAuthorizedS3Actions {

 private
  static final String RESTRICTED_PUBLIC_ACCESS_ACTIONS =
      "/PolicyAuthorizedS3Actions.json";

 private
  static Set<String> actionsSet = new HashSet<>();
 private
  static final Logger LOGGER =
      LoggerFactory.getLogger(PolicyAuthorizedS3Actions.class.getName());
 private
  static PolicyAuthorizedS3Actions instance = null;

 private
  PolicyAuthorizedS3Actions() { init(); }

 public
  static PolicyAuthorizedS3Actions getInstance() {
    if (instance == null) {
      instance = new PolicyAuthorizedS3Actions();
    }
    return instance;
  }

 public
  static void init() {
    try {
      InputStream in = PolicyAuthorizedS3Actions.class.getResourceAsStream(
          RESTRICTED_PUBLIC_ACCESS_ACTIONS);
      InputStreamReader reader = new InputStreamReader(in, "UTF-8");
      JsonParser jsonParser = new JsonParser();
      JsonObject element = (JsonObject)jsonParser.parse(reader);
      for (JsonElement action : element.get("actions").getAsJsonArray()) {
        actionsSet.add(action.getAsString());
      }
    }
    catch (IOException e) {
      LOGGER.error("Exception during json parsing", e);
    }
  }

 public
  boolean isOnlyPolicyAuthorizationRequired(String action) {
    return actionsSet.contains(action);
  }
}

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
import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.reflect.TypeToken;

public
class S3Actions {

 private
  static final String S3_ACTIONS_FILE = "/S3Actions.json";

 private
  Map<String, Set<String>> bucketOperations = new HashMap<>();
 private
  Map<String, Set<String>> objectOperations = new HashMap<>();

 private
  S3Actions() { init(); }

 public
  void init() {
    InputStream in = null;
    in = S3Actions.class.getResourceAsStream(S3_ACTIONS_FILE);
    InputStreamReader reader = new InputStreamReader(in);
    JsonParser jsonParser = new JsonParser();
    JsonObject element = (JsonObject)jsonParser.parse(reader);
    Type setType = new TypeToken<Set<String>>() {}
    .getType();
    Gson gson = new Gson();
    Set<String> keys = new HashSet<>();

    for (Entry<String, JsonElement>
             entry : element.get("Bucket").getAsJsonObject().entrySet()) {
      JsonElement setElem = entry.getValue();
      keys = gson.fromJson(setElem, setType);
      bucketOperations.put(entry.getKey().toLowerCase(), keys);
    }

    for (Entry<String, JsonElement>
             entry : element.get("Object").getAsJsonObject().entrySet()) {
      JsonElement setElem = entry.getValue();
      keys = gson.fromJson(setElem, setType);
      objectOperations.put(entry.getKey().toLowerCase(), keys);
    }
    try {
      if (reader != null) reader.close();
      if (in != null) in.close();
    }
    catch (IOException e) {
      // Do nothing
    }
  }

 private
  static class S3ActionsHolder {
    static final S3Actions S3ACTIONS_INSTANCE = new S3Actions();
  } public static S3Actions getInstance() {
    return S3ActionsHolder.S3ACTIONS_INSTANCE;
  }

 public
  Map<String, Set<String>> getBucketOperations() { return bucketOperations; }

 public
  Map<String, Set<String>> getObjectOperations() { return objectOperations; }
}

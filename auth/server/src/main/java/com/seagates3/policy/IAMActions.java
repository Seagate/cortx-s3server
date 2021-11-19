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
class IAMActions {

 private
  static final String IAM_ACTIONS_FILE = "/IAMActions.json";

 private
  Map<String, Set<String>> operations = new HashMap<>();

 private
  IAMActions() { init(); }

 public
  void init() {
    InputStream in = null;
    in = IAMActions.class.getResourceAsStream(IAM_ACTIONS_FILE);
    InputStreamReader reader = new InputStreamReader(in);
    JsonParser jsonParser = new JsonParser();
    JsonObject element = (JsonObject)jsonParser.parse(reader);
    Type setType = new TypeToken<Set<String>>() {}
    .getType();
    Gson gson = new Gson();
    Set<String> keys = new HashSet<>();

    for (Entry<String, JsonElement> entry :
         element.getAsJsonObject().entrySet()) {
      JsonElement setElem = entry.getValue();
      keys = gson.fromJson(setElem, setType);
      operations.put(entry.getKey().toLowerCase(), keys);
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
  static class IAMActionsHolder {
    static final IAMActions IAMACTIONS_INSTANCE = new IAMActions();
  } public static IAMActions getInstance() {
    return IAMActionsHolder.IAMACTIONS_INSTANCE;
  }

 public
  Map<String, Set<String>> getOperations() { return operations; }
}

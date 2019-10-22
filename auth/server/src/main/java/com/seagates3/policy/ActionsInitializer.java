package com.seagates3.policy;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;

import com.google.gson.Gson;

public
class ActionsInitializer {

 private
  static final String VALID_ACTIONS_FILE = "/Actions.json";

 private
  static HashMap<String, String> actionsMap;

 public
  static void init() throws UnsupportedEncodingException {
    InputStream in =
        ActionsInitializer.class.getResourceAsStream(VALID_ACTIONS_FILE);
    InputStreamReader reader = new InputStreamReader(in, "UTF-8");

    Gson gson = new Gson();
    actionsMap = gson.fromJson(reader, HashMap.class);
  }

 public
  static HashMap<String, String> getActionsMap() { return actionsMap; }
}

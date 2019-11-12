package com.seagates3.policy;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;

import com.google.gson.Gson;

public
class ActionsInitializer {

 private
  static final String S3_ACTIONS_FILE = "/S3Actions.json";

 private
  static final String IAM_ACTIONS_FILE = "/IAMActions.json";

 private
  static HashMap<String, String> actionsMap;

 public
  static void init(PolicyUtil.Services service)
      throws UnsupportedEncodingException {
    InputStream in = null;
    switch (service) {
      case S3:
        in = ActionsInitializer.class.getResourceAsStream(S3_ACTIONS_FILE);
        break;

      case IAM:
        in = ActionsInitializer.class.getResourceAsStream(IAM_ACTIONS_FILE);
        break;

      default:
        break;
    }
    InputStreamReader reader = new InputStreamReader(in, "UTF-8");

    Gson gson = new Gson();
    actionsMap = gson.fromJson(reader, HashMap.class);
  }

 public
  static HashMap<String, String> getActionsMap() { return actionsMap; }
}

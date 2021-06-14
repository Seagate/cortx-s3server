package com.seagates3.authorization;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public
class PublicAccessAuthorizer {

 private
  static final String RESTRICTED_PUBLIC_ACCESS_ACTIONS =
      "/RestrictedPublicAccessS3Actions.json";

 private
  static Set<String> actionsList = new HashSet<>();
 private
  static final Logger LOGGER =
      LoggerFactory.getLogger(PublicAccessAuthorizer.class.getName());
 private
  static PublicAccessAuthorizer instance = null;
 private
  PublicAccessAuthorizer() { init(); }
 public
  static PublicAccessAuthorizer getInstance() {
    if (instance == null) {
      instance = new PublicAccessAuthorizer();
    }
    return instance;
  }
 public
  static void init() {
    try {
      InputStream in = PublicAccessAuthorizer.class.getResourceAsStream(
          RESTRICTED_PUBLIC_ACCESS_ACTIONS);
      InputStreamReader reader = new InputStreamReader(in, "UTF-8");
      JsonParser jsonParser = new JsonParser();
      JsonObject element = (JsonObject)jsonParser.parse(reader);
      for (JsonElement action : element.get("actions").getAsJsonArray()) {
        actionsList.add(action.getAsString());
      }
    }
    catch (IOException e) {
      LOGGER.error("Exception during json parsing", e);
    }
  }

 public
  boolean isActionRestricted(Map<String, String> requestBody) {
    return actionsList.contains(getActionType(requestBody));
  }

 private
  String getActionType(Map<String, String> requestBody) {
    String action = requestBody.get("S3Action");
    if (requestBody.get("x-amz-copy-source") != null &&
        "PutObject".equals(action)) {
      action = "CopyObject";
    }
    return action;
  }
}

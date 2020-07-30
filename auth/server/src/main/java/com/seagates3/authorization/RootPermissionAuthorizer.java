package com.seagates3.authorization;

import java.util.HashSet;
import java.util.Set;

public
class RootPermissionAuthorizer {

 private
  static RootPermissionAuthorizer instance = null;
 private
  Set<String> actionsSet = new HashSet<>();

 private
  RootPermissionAuthorizer() {
    actionsSet.add("CreateLoginProfile");
    actionsSet.add("UpdateLoginProfile");
    actionsSet.add("GetLoginProfile");
    actionsSet.add("CreateAccountLoginProfile");
    actionsSet.add("GetAccountLoginProfile");
    actionsSet.add("UpdateAccountLoginProfile");
  }

 public
  static RootPermissionAuthorizer getInstance() {
    if (instance == null) {
      instance = new RootPermissionAuthorizer();
    }
    return instance;
  }

 public
  boolean containsAction(String action) {
    return actionsSet.contains(action) ? true : false;
  }
}

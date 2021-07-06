package com.seagates3.service;

import java.util.HashMap;
import java.util.Map;

import com.seagates3.model.GlobalData;

public
class GlobalDataStore {

 private
  static GlobalDataStore instance;
 private
  Map<String, GlobalData> authenticationMap;

 private
  GlobalDataStore() { authenticationMap = new HashMap<>(); }

 public
  static GlobalDataStore getInstance() {
    if (instance == null) {
      instance = new GlobalDataStore();
    }
    return instance;
  }

 public
  Map<String, GlobalData> getAuthenticationMap() { return authenticationMap; }

 public
  void addToAuthenticationMap(String accessKey, GlobalData globalObj) {
    this.authenticationMap.put(accessKey, globalObj);
  }
}

package com.seagates3.dao.ldap;

public
class FileStoreUtil {

 public
  static String retrieveKeyFromArn(String arn) {
    String key = null;
    String token[] = arn.split(":");
    String account_id = token[4];
    String policyName = token[5].split("/")[1];
    key = policyName + "#" + account_id;
    return key;
  }

 public
  static String getKey(String policyName, String accountid) {
    return policyName + "#" + accountid;
  }
}

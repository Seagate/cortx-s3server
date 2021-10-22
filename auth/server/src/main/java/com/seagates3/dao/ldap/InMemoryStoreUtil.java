package com.seagates3.dao.ldap;

public
class InMemoryStoreUtil {

 public
  static String retrieveKeyFromArn(String arn) {
    String key = null;
    String token[] = arn.split(":");
    String account_id = token[4];
    String policy_id = token[5].split("/")[1];
    key = policy_id + "#" + account_id;
    return key;
  }

 public
  static String getKey(String policyid, String accountid) {
    return policyid + "#" + accountid;
  }
}

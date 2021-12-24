package com.seagates3.dao.ldap;

import com.seagates3.authserver.AuthServerConfig;

public
class AuthStoreFactory {

 public
  AuthStore createAuthStore(String prefix) {
    String store = AuthServerConfig.getAuthStore();
    if ("MOTR".equals(store)) {
      return new MotrStore();
    } else if ("LDAP".equals(store)) {
      return new LdapStore();
    } else if ("FILE".equals(store)) {
      return new FileStore(prefix);
    } else {
      return null;
    }
  }
}

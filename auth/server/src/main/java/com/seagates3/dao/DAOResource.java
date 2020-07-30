package com.seagates3.dao;

/*
 * TODO
 * Rename the enum name
 */
public
 enum DAOResource {
   ACCESS_KEY("AccessKey"),
   ACCOUNT("Account"),
   FED_USER("FedUser"),
   GROUP("Group"),
   POLICY("Policy"),
   REQUESTEE("Requestee"),
   REQUESTOR("Requestor"),
   ROLE("Role"),
   SAML_PROVIDER("SAMLProvider"),
   USER("User"),
   USER_LOGIN_PROFILE("UserLoginProfile"),
   ACCOUNT_LOGIN_PROFILE("AccountLoginProfile");
   private final String className;
   private DAOResource(final String className) {this.className = className;}

    @Override
    public String toString() {
        return className;
    }
}

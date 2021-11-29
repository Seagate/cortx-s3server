package com.seagates3.dao.ldap;

import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;
import com.seagates3.model.User;
import com.seagates3.model.UserPolicy;

public
class UserPolicyLdapStore {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(UserPolicyLdapStore.class.getName());

 public
  void attach(Object userPolicyObj) throws DataAccessException {
    UserPolicy userPolicy = (UserPolicy)userPolicyObj;
    User user = userPolicy.getUser();
    Policy policy = userPolicy.getPolicy();

    ArrayList modList = new ArrayList();
    LDAPAttribute attr;

    String dn =
        String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.USER_ID,
                      user.getId(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.USER_OU, LDAPUtils.ORGANIZATIONAL_NAME,
                      user.getAccountName(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN);

    if (policy.getPolicyId() != null) {
      List<String> userPolicyIds = user.getPolicyIds();
      userPolicyIds.add(policy.getPolicyId());
      String[] policyIds = new String[userPolicyIds.size()];
      userPolicyIds.toArray(policyIds);
      attr = new LDAPAttribute(LDAPUtils.POLICY_ID, policyIds);
      modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
    }

    LOGGER.debug("Updating user dn: " + dn + " user name: " + user.getName() +
                 " with policy id: " + policy.getPolicyId());

    try {
      LDAPUtils.modify(dn, modList);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to modify the details of user: " + user.getName());
      throw new DataAccessException("Failed to modify the user" +
                                    " details.\n" + ex);
    }
  }

 public
  void detach(Object userPolicyObj) throws DataAccessException {
    UserPolicy userPolicy = (UserPolicy)userPolicyObj;
    User user = userPolicy.getUser();
    Policy policy = userPolicy.getPolicy();

    ArrayList modList = new ArrayList();
    LDAPAttribute attr;

    String dn =
        String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.USER_ID,
                      user.getId(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.USER_OU, LDAPUtils.ORGANIZATIONAL_NAME,
                      user.getAccountName(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN);

    if (policy.getPolicyId() != null) {
      List<String> userPolicyIds = user.getPolicyIds();
      userPolicyIds.remove(policy.getPolicyId());
      String[] policyIds = new String[userPolicyIds.size()];
      userPolicyIds.toArray(policyIds);
      attr = new LDAPAttribute(LDAPUtils.POLICY_ID, policyIds);
      modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
    }

    LOGGER.debug("Updating user dn: " + dn + " user name: " + user.getName() +
                 " with policy id: " + policy.getPolicyId());

    try {
      LDAPUtils.modify(dn, modList);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to detach policy from the user: " + user.getName());
      throw new DataAccessException("Failed to modify the user" +
                                    " details.\n" + ex);
    }
  }
}

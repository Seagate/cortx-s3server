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

    String userDN = getUserDN(user);
    String policyDN = getPolicyDN(policy.getName(), user.getAccountName());

    LOGGER.debug("Attaching policy [" + policy.getPolicyId() + "] to user [" +
                 user.getName() + "]");

    ArrayList userModList =
        getUserModListForAttachDetachPolicy(user, policy, true);
    ArrayList policyModList =
        getPolicyModListForAttachDetachPolicy(policy, true);

    try {
      LDAPUtils.modify(userDN, userModList);
      LDAPUtils.modify(policyDN, policyModList);
    }
    catch (LDAPException ex) {
      String errMsg = "Failed to attach policy [" + policy.getPolicyId() +
                      "] to user [" + user.getName() + "]";
      LOGGER.error(errMsg);
      throw new DataAccessException(errMsg + ". " + ex.getMessage());
    }
  }

 public
  void detach(Object userPolicyObj) throws DataAccessException {
    UserPolicy userPolicy = (UserPolicy)userPolicyObj;
    User user = userPolicy.getUser();
    Policy policy = userPolicy.getPolicy();

    String userDN = getUserDN(user);
    String policyDN = getPolicyDN(policy.getName(), user.getAccountName());

    LOGGER.debug("Detaching policy [" + policy.getPolicyId() + "] from user [" +
                 user.getName() + "]");

    ArrayList userModList =
        getUserModListForAttachDetachPolicy(user, policy, false);
    ArrayList policyModList =
        getPolicyModListForAttachDetachPolicy(policy, false);

    try {
      LDAPUtils.modify(userDN, userModList);
      LDAPUtils.modify(policyDN, policyModList);
    }
    catch (LDAPException ex) {
      String errMsg = "Failed to detach policy [" + policy.getPolicyId() +
                      "] from user [" + user.getName() + "]";
      LOGGER.error(errMsg);
      throw new DataAccessException(errMsg + ". " + ex.getMessage());
    }
  }

 private
  String getUserDN(User user) {
    return String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.USER_ID,
                         user.getId(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                         LDAPUtils.USER_OU, LDAPUtils.ORGANIZATIONAL_NAME,
                         user.getAccountName(),
                         LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                         LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN);
  }

 private
  String getPolicyDN(String policyName, String accountName) {
    return String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.POLICY_NAME,
                         policyName, LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                         LDAPUtils.POLICY_OU, LDAPUtils.ORGANIZATIONAL_NAME,
                         accountName, LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                         LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN);
  }

 private
  ArrayList getUserModListForAttachDetachPolicy(User user, Policy policy,
                                                boolean isAttach) {
    ArrayList userModList = new ArrayList();
    List<String> userPolicyIds = user.getPolicyIds();
    if (isAttach) {
      userPolicyIds.add(policy.getPolicyId());
    } else {
      userPolicyIds.remove(policy.getPolicyId());
    }
    String[] policyIds = new String[userPolicyIds.size()];
    userPolicyIds.toArray(policyIds);
    LDAPAttribute attr = new LDAPAttribute(LDAPUtils.POLICY_ID, policyIds);
    userModList.add(new LDAPModification(LDAPModification.REPLACE, attr));

    return userModList;
  }

 private
  ArrayList getPolicyModListForAttachDetachPolicy(Policy policy,
                                                  boolean isAttach) {
    String newAttachmentCount =
        isAttach ? String.valueOf(policy.getAttachmentCount() + 1)
                 : String.valueOf(policy.getAttachmentCount() - 1);
    ArrayList policyModList = new ArrayList();
    LDAPAttribute attr = new LDAPAttribute(LDAPUtils.POLICY_ATTACHMENT_COUNT,
                                           newAttachmentCount);
    policyModList.add(new LDAPModification(LDAPModification.REPLACE, attr));

    return policyModList;
  }
}

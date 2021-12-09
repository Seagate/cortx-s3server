package com.seagates3.dao.ldap;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Date;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;
import com.seagates3.util.DateUtil;

public
class PolicyLdapStore {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyLdapStore.class.getName());

 public
  void save(Object policyObj) throws DataAccessException {
    Policy policy = (Policy)policyObj;
    LDAPAttributeSet attributeSet = new LDAPAttributeSet();
    attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                                       LDAPUtils.POLICY_OBJECT_CLASS));
    attributeSet.add(
        new LDAPAttribute(LDAPUtils.POLICY_NAME, policy.getName()));
    attributeSet.add(
        new LDAPAttribute(LDAPUtils.POLICY_DOC, policy.getPolicyDoc()));
    attributeSet.add(new LDAPAttribute(LDAPUtils.PATH, policy.getPath()));
    attributeSet.add(new LDAPAttribute(LDAPUtils.DEFAULT_VERSION_ID,
                                       policy.getDefaultVersionid()));
    attributeSet.add(
        new LDAPAttribute(LDAPUtils.POLICY_ID, policy.getPolicyId()));
    attributeSet.add(new LDAPAttribute(LDAPUtils.POLICY_ARN, policy.getARN()));
    attributeSet.add(new LDAPAttribute(LDAPUtils.POLICY_ATTACHMENT_COUNT, "0"));
    attributeSet.add(new LDAPAttribute(
        LDAPUtils.POLICY_CREATE_DATE,
        DateUtil.toLdapDate(new Date(DateUtil.getCurrentTime()))));
    attributeSet.add(new LDAPAttribute(
        LDAPUtils.POLICY_UPDATE_DATE,
        DateUtil.toLdapDate(new Date(DateUtil.getCurrentTime()))));
    attributeSet.add(
        new LDAPAttribute(LDAPUtils.POLICY_PERMISSION_BOUNDARY, "0"));
    attributeSet.add(new LDAPAttribute(LDAPUtils.IS_POLICY_ATTACHABLE, "true"));

    if (policy.getDescription() != null) {
      attributeSet.add(
          new LDAPAttribute(LDAPUtils.DESCRIPTION, policy.getDescription()));
    }
    String dn = String.format(
        "%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.POLICY_NAME, policy.getName(),
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.POLICY_OU,
        LDAPUtils.ORGANIZATIONAL_NAME, policy.getAccount().getName(),
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
        LDAPUtils.BASE_DN);

    LOGGER.debug("Saving Policy dn: " + dn);
    try {
      LDAPUtils.add(new LDAPEntry(dn, attributeSet));
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to create policy: " + policy.getName());
      throw new DataAccessException("Failed to create policy.\n" + ex);
    }
  }

 public
  Policy find(Object policyArnStr,
              Object accountObj) throws DataAccessException {
    String policyarn = policyArnStr.toString();
    Account requestingAccount = (Account)accountObj;
    Policy policy = new Policy();
    policy.setARN(policyarn);

    String[] attrs = {
        LDAPUtils.POLICY_ID,                 LDAPUtils.PATH,
        LDAPUtils.POLICY_NAME,               LDAPUtils.POLICY_CREATE_DATE,
        LDAPUtils.POLICY_UPDATE_DATE,        LDAPUtils.DEFAULT_VERSION_ID,
        LDAPUtils.POLICY_DOC,                LDAPUtils.IS_POLICY_ATTACHABLE,
        LDAPUtils.POLICY_ARN,                LDAPUtils.POLICY_ATTACHMENT_COUNT,
        LDAPUtils.POLICY_PERMISSION_BOUNDARY};
    String baseDN = String.format(
        "%s=%s,%s=%s,%s=%s,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
        LDAPUtils.POLICY_OU, LDAPUtils.ORGANIZATIONAL_NAME,
        requestingAccount.getName(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
        LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN);
    String filter = String.format("(%s=%s)", LDAPUtils.POLICY_ARN, policyarn);
    LDAPSearchResults ldapResults;
    LOGGER.debug("Searching policy dn: " + baseDN + " filter: " + filter);

    try {
      ldapResults =
          LDAPUtils.search(baseDN, LDAPConnection.SCOPE_SUB, filter, attrs);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to find the policy: " + policy.getName() +
                   " filter: " + filter);
      throw new DataAccessException("Failed to find the policy.\n" + ex);
    }
    if (ldapResults != null && ldapResults.hasMore()) {
      try {
        LDAPEntry entry = ldapResults.next();
        policy.setPolicyId(
            entry.getAttribute(LDAPUtils.POLICY_ID).getStringValue());
        policy.setPath(entry.getAttribute(LDAPUtils.PATH).getStringValue());
        policy.setDefaultVersionId(
            entry.getAttribute(LDAPUtils.DEFAULT_VERSION_ID).getStringValue());
        policy.setPolicyDoc(
            entry.getAttribute(LDAPUtils.POLICY_DOC).getStringValue());
        policy.setName(
            entry.getAttribute(LDAPUtils.POLICY_NAME).getStringValue());
        policy.setARN(
            entry.getAttribute(LDAPUtils.POLICY_ARN).getStringValue());
        policy.setAttachmentCount(Integer.parseInt(
            entry.getAttribute(LDAPUtils.POLICY_ATTACHMENT_COUNT)
                .getStringValue()));
        policy.setIsPolicyAttachable(
            entry.getAttribute(LDAPUtils.IS_POLICY_ATTACHABLE)
                .getStringValue());
        policy.setPermissionsBoundaryUsageCount(Integer.parseInt(
            entry.getAttribute(LDAPUtils.POLICY_PERMISSION_BOUNDARY)
                .getStringValue()));
        String createTimeStamp =
            entry.getAttribute(LDAPUtils.POLICY_CREATE_DATE).getStringValue();
        String createTime = DateUtil.toServerResponseFormat(createTimeStamp);
        policy.setCreateDate(createTime);

        String modifyTimeStamp =
            entry.getAttribute(LDAPUtils.POLICY_UPDATE_DATE).getStringValue();
        String modifiedTime = DateUtil.toServerResponseFormat(modifyTimeStamp);
        policy.setUpdateDate(modifiedTime);
      }
      catch (LDAPException ex) {
        LOGGER.error("Failed to find details of policy: " + policy.getName());
        throw new DataAccessException("Failed to find policy details. \n" + ex);
      }
    }
    return policy;
  }

 public
  List<Policy> findAll(Object accountObj,
                       Object parametersObj) throws DataAccessException {
    Account account = (Account)accountObj;
    Map<String, Object> parameters = (Map<String, Object>)parametersObj;
    String optionalFilter = "";
    String[] attrs = {LDAPUtils.POLICY_ID,
                      LDAPUtils.PATH,
                      LDAPUtils.POLICY_CREATE_DATE,
                      LDAPUtils.POLICY_UPDATE_DATE,
                      LDAPUtils.DEFAULT_VERSION_ID,
                      LDAPUtils.POLICY_NAME,
                      LDAPUtils.IS_POLICY_ATTACHABLE,
                      LDAPUtils.POLICY_ARN,
                      LDAPUtils.POLICY_ATTACHMENT_COUNT,
                      LDAPUtils.POLICY_PERMISSION_BOUNDARY};
    String ldapBase = String.format(
        "%s=%s,%s=%s,%s=%s,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
        LDAPUtils.POLICY_OU, LDAPUtils.ORGANIZATIONAL_NAME, account.getName(),
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
        LDAPUtils.BASE_DN);

    String filter = "(" + LDAPUtils.OBJECT_CLASS + "=" +
                    LDAPUtils.POLICY_OBJECT_CLASS + ")";
    if (!parameters.isEmpty()) {
      if (parameters.get("PathPrefix") != null) {
        optionalFilter += "(" + LDAPUtils.PATH + "=" +
                          (String)parameters.get("PathPrefix") + ")";
      }
      if (parameters.get("OnlyAttached") != null) {
        String onlyAttachedValue = (String)parameters.get("OnlyAttached");
        if (onlyAttachedValue.equals("true")) {
          optionalFilter +=
              "(!(" + LDAPUtils.POLICY_ATTACHMENT_COUNT + "=0" + "))";
        }
      }
      if (parameters.get("PolicyName") != null) {
        optionalFilter += "(" + LDAPUtils.POLICY_NAME + "=" +
                          (String)parameters.get("PolicyName") + ")";
      }
    }
    filter = "(&" + filter + optionalFilter + ")";

    LDAPSearchResults ldapResults;
    LOGGER.debug("Searching policy dn: " + ldapBase + " filter: " + filter);
    try {
      ldapResults =
          LDAPUtils.search(ldapBase, LDAPConnection.SCOPE_SUB, filter, attrs);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to find the policies: ");
      throw new DataAccessException("Failed to find the policies.\n" + ex);
    }
    List<Policy> resultList = new ArrayList<>();
    if (ldapResults != null) {
      while (ldapResults.hasMore()) {

        try {
          Policy policy = new Policy();
          policy.setAccount(account);
          LDAPEntry entry = ldapResults.next();
          policy.setPolicyId(
              entry.getAttribute(LDAPUtils.POLICY_ID).getStringValue());
          policy.setPath(entry.getAttribute(LDAPUtils.PATH).getStringValue());
          policy.setDefaultVersionId(
              entry.getAttribute(LDAPUtils.DEFAULT_VERSION_ID)
                  .getStringValue());
          policy.setName(
              entry.getAttribute(LDAPUtils.POLICY_NAME).getStringValue());
          policy.setARN(
              entry.getAttribute(LDAPUtils.POLICY_ARN).getStringValue());
          policy.setAttachmentCount(Integer.parseInt(
              entry.getAttribute(LDAPUtils.POLICY_ATTACHMENT_COUNT)
                  .getStringValue()));
          policy.setIsPolicyAttachable(
              entry.getAttribute(LDAPUtils.IS_POLICY_ATTACHABLE)
                  .getStringValue());
          policy.setPermissionsBoundaryUsageCount(Integer.parseInt(
              entry.getAttribute(LDAPUtils.POLICY_PERMISSION_BOUNDARY)
                  .getStringValue()));

          String createTimeStamp =
              entry.getAttribute(LDAPUtils.POLICY_CREATE_DATE).getStringValue();
          String createTime = DateUtil.toServerResponseFormat(createTimeStamp);
          policy.setCreateDate(createTime);

          String modifyTimeStamp =
              entry.getAttribute(LDAPUtils.POLICY_UPDATE_DATE).getStringValue();
          String modifiedTime =
              DateUtil.toServerResponseFormat(modifyTimeStamp);
          policy.setUpdateDate(modifiedTime);
          resultList.add(policy);
        }
        catch (LDAPException ex) {
          LOGGER.error("Failed to find policies");
          throw new DataAccessException("Failed to find policies. \n" + ex);
        }
      }
    }
    return resultList;
  }

 public
  void delete (Object policyObj) throws DataAccessException {
    Policy policy = (Policy)policyObj;
    String dn = String.format(
        "%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.POLICY_NAME, policy.getName(),
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.POLICY_OU,
        LDAPUtils.ORGANIZATIONAL_NAME, policy.getAccount().getName(),
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
        LDAPUtils.BASE_DN);
    LOGGER.debug("Deleting user dn: " + dn);
    try {
      LDAPUtils.delete (dn);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to delete the policy: " + policy.getName());
      throw new DataAccessException("Failed to delete the policy.\n" + ex);
    }
  }
}

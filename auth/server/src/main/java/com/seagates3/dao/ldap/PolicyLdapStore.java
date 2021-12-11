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
import com.seagates3.constants.APIRequestParamsConstants;
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
        policy = setPolicyFromLdapEntry(entry);
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
                       Object apiParametersObj) throws DataAccessException {
    Account account = (Account)accountObj;
    Map<String, Object> apiParameters = (Map<String, Object>)apiParametersObj;
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
    if (!apiParameters.isEmpty()) {
      optionalFilter = prepareOptionalFilter(apiParameters);
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

          LDAPEntry entry = ldapResults.next();
          Policy policy = setPolicyFromLdapEntry(entry);
          policy.setAccount(account);
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

 public
  List<Policy> findByIds(Map<String, Object> dataMap)
      throws DataAccessException {
    LDAPSearchResults ldapResults = getPolicyByIdsLdapResults(dataMap);
    List<Policy> resultList = new ArrayList<>();
    if (ldapResults != null) {
      while (ldapResults.hasMore()) {
        try {
          LDAPEntry entry = ldapResults.next();
          Policy policy = setPolicyFromLdapEntry(entry);
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

 private
  String getStringValueFromLdapAttribute(LDAPAttribute attribute) {
    return attribute != null ? attribute.getStringValue() : "";
  }

 private
  Policy setPolicyFromLdapEntry(LDAPEntry entry) {
    Policy policy = new Policy();
    policy.setPolicyId(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_ID)));
    policy.setPath(
        getStringValueFromLdapAttribute(entry.getAttribute(LDAPUtils.PATH)));
    policy.setDefaultVersionId(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.DEFAULT_VERSION_ID)));
    policy.setPolicyDoc(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_DOC)));
    policy.setName(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_NAME)));
    policy.setARN(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_ARN)));
    policy.setAttachmentCount(Integer.parseInt(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_ATTACHMENT_COUNT))));
    policy.setIsPolicyAttachable(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.IS_POLICY_ATTACHABLE)));
    policy.setPermissionsBoundaryUsageCount(
        Integer.parseInt(getStringValueFromLdapAttribute(
            entry.getAttribute(LDAPUtils.POLICY_PERMISSION_BOUNDARY))));
    policy.setDescription(getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.DESCRIPTION)));

    String createTimeStamp = getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_CREATE_DATE));
    String createTime = DateUtil.toServerResponseFormat(createTimeStamp);
    policy.setCreateDate(createTime);

    String modifyTimeStamp = getStringValueFromLdapAttribute(
        entry.getAttribute(LDAPUtils.POLICY_UPDATE_DATE));
    String modifiedTime = DateUtil.toServerResponseFormat(modifyTimeStamp);
    policy.setUpdateDate(modifiedTime);
    return policy;
  }

 private
  LDAPSearchResults getPolicyByIdsLdapResults(Map<String, Object> dataMap)
      throws DataAccessException {
    LDAPSearchResults ldapResults = null;
    String accountName = dataMap.get("accountName") != null
                             ? (String)dataMap.get("accountName")
                             : "";
    List<String> policyIds = dataMap.get("policyIds") != null
                                 ? (List<String>)dataMap.get("policyIds")
                                 : null;

    String[] attrs = {
        LDAPUtils.POLICY_ID,                 LDAPUtils.PATH,
        LDAPUtils.POLICY_CREATE_DATE,        LDAPUtils.POLICY_UPDATE_DATE,
        LDAPUtils.DEFAULT_VERSION_ID,        LDAPUtils.POLICY_DOC,
        LDAPUtils.POLICY_NAME,               LDAPUtils.IS_POLICY_ATTACHABLE,
        LDAPUtils.POLICY_ARN,                LDAPUtils.POLICY_ATTACHMENT_COUNT,
        LDAPUtils.POLICY_PERMISSION_BOUNDARY};

    String ldapBase = String.format(
        "%s=%s,%s=%s,%s=%s,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
        LDAPUtils.POLICY_OU, LDAPUtils.ORGANIZATIONAL_NAME, accountName,
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
        LDAPUtils.BASE_DN);
    String filter = "(" + LDAPUtils.OBJECT_CLASS + "=" +
                    LDAPUtils.POLICY_OBJECT_CLASS + ")";
    String policyIdFilter = "";
    if (policyIds != null && !policyIds.isEmpty()) {
      for (String policyId : policyIds) {
        policyIdFilter += "(policyId=" + policyId + ")";
      }
      if (policyIds.size() > 1) {
        policyIdFilter = "(|" + policyIdFilter + ")";
      }
    }
    String optionalFilter = "";
    if (dataMap.get(APIRequestParamsConstants.PATH_PREFIX) != null) {
      optionalFilter =
          "(" + LDAPUtils.PATH + "=" + (String)dataMap.get(APIRequestParamsConstants.PATH_PREFIX) + ")";
    }

    if (policyIdFilter.length() > 0) {
      filter = "(&" + filter + policyIdFilter + optionalFilter + ")";

      LOGGER.debug("Searching policy dn: " + ldapBase + " filter: " + filter);
      try {
        ldapResults =
            LDAPUtils.search(ldapBase, LDAPConnection.SCOPE_SUB, filter, attrs);
      }
      catch (LDAPException ex) {
        LOGGER.error("Failed to find the policies: ");
        throw new DataAccessException("Failed to find the policies.\n" + ex);
      }
    }
    return ldapResults;
  }
 private
  String prepareOptionalFilter(Map<String, Object> apiParameters) {
    final String TRUE = "true";
    String optionalFilter = "";

    if (apiParameters.get(APIRequestParamsConstants.PATH_PREFIX) != null) {
      optionalFilter +=
          "(" + LDAPUtils.PATH + "=" +
          (String)apiParameters.get(APIRequestParamsConstants.PATH_PREFIX) +
          ")";
    }
    if (apiParameters.get(APIRequestParamsConstants.ONLY_ATTACHED) != null) {
      String onlyAttachedValue =
          (String)apiParameters.get(APIRequestParamsConstants.ONLY_ATTACHED);
      if (onlyAttachedValue.equals(TRUE)) {
        optionalFilter +=
            "(!(" + LDAPUtils.POLICY_ATTACHMENT_COUNT + "=0" + "))";
      }
    }
    if (apiParameters.get(APIRequestParamsConstants.POLICY_NAME) != null) {
      optionalFilter +=
          "(" + LDAPUtils.POLICY_NAME + "=" +
          (String)apiParameters.get(APIRequestParamsConstants.POLICY_NAME) +
          ")";
    }

    return optionalFilter;
  }
}

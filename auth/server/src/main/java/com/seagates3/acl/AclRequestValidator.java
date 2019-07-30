/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Shalaka Dharap
 * Original creation date: 24-July-2019
 */

package com.seagates3.acl;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;

public
class AclRequestValidator {

 protected
  final Logger LOGGER =
      LoggerFactory.getLogger(AclRequestValidator.class.getName());
 protected
  AuthorizationResponseGenerator responseGenerator;
 protected
  static final Set<String> permissionHeadersSet = new HashSet<>();
 protected
  static final Set<String> cannedAclSet = new HashSet<>();

 public
  AclRequestValidator() {
    responseGenerator = new AuthorizationResponseGenerator();
    initializePermissionHeadersSet();
    initializeCannedAclSet();
  }

  /**
   * Below method will initialize the set with valid permission headers
   */
 private
  void initializePermissionHeadersSet() {
    permissionHeadersSet.add("x-amz-grant-read");
    permissionHeadersSet.add("x-amz-grant-write");
    permissionHeadersSet.add("x-amz-grant-read-acp");
    permissionHeadersSet.add("x-amz-grant-write-acp");
    permissionHeadersSet.add("x-amz-grant-full-control");
  }

  /**
   * Below method will initialize the set with valid canned acls
   */
 private
  void initializeCannedAclSet() {
    cannedAclSet.add("private");
    cannedAclSet.add("public-read");
    cannedAclSet.add("public-read-write");
    cannedAclSet.add("authenticated-read");
    cannedAclSet.add("bucket-owner-read");
    cannedAclSet.add("bucket-owner-full-control");
    cannedAclSet.add("log-delivery-write");
  }

  /**
   * Below method will first check if only one of the below options is used in
   * request- 1. canned acl 2. permission header 3. acl in requestbody and then
   * method will perform requested account validation.
   *
   * @param requestBody
   * @return null- if request valid and errorResponse- if request not valid
   */
 public
  ServerResponse validateAclRequest(
      Map<String, String> requestBody,
      Map<String, List<Account>> accountPermissionMap) {
    int isCannedAclPresent = 0, isPermissionHeaderPresent = 0,
        isAclPresentInRequestBody = 0;
    ServerResponse response = null;
    // Check if canned ACL is present in request
    if (requestBody.get("x-amz-acl") != null) {
      isCannedAclPresent = 1;
    }
    // Check if permission header are present
    if (requestBody.get("x-amz-grant-read") != null ||
        requestBody.get("x-amz-grant-write") != null ||
        requestBody.get("x-amz-grant-read-acp") != null ||
        requestBody.get("x-amz-grant-write-acp") != null ||
        requestBody.get("x-amz-grant-full-control") != null) {
      isPermissionHeaderPresent = 1;
    }
    // TODO Check for the attribute key here
    if (requestBody.get("acp") != null) {
      isAclPresentInRequestBody = 1;
    }
    if ((isCannedAclPresent + isPermissionHeaderPresent +
         isAclPresentInRequestBody) > 1) {
      response = responseGenerator.badRequest();
      LOGGER.error(
          response.getResponseStatus() +
          " : Request failed: More than one options are used in request");
    }
    if (response == null) {
      if (isPermissionHeaderPresent == 1) {
        isValidPermissionHeader(requestBody, response, accountPermissionMap);
      } else if (isCannedAclPresent == 1) {
        isValidCannedAcl(requestBody, response);
      }
    }

    return response;
  }

 protected
  boolean isValidCannedAcl(Map<String, String> requestBody,
                           ServerResponse response) {
    boolean isValid = true;
    if (!cannedAclSet.contains(requestBody.get("x-amz-acl"))) {
      isValid = false;
    }
    return isValid;
  }

  /**
   * Below method will validate the requested account
   *
   * @param requestBody
   * @return
   */
 protected
  boolean isValidPermissionHeader(
      Map<String, String> requestBody, ServerResponse response,
      Map<String, List<Account>> accountPermissionMap) {
    boolean isValid = true;
    for (String permissionHeader : permissionHeadersSet) {
      if (requestBody.get(permissionHeader) != null) {
        StringTokenizer tokenizer =
            new StringTokenizer(requestBody.get(permissionHeader), ",");
        List<String> valueList = new ArrayList<>();
        while (tokenizer.hasMoreTokens()) {
          valueList.add(tokenizer.nextToken());
        }
        for (String value : valueList) {
          if (!isGranteeValid(value, response, accountPermissionMap,
                              permissionHeader)) {
            LOGGER.error(response.getResponseStatus() +
                         " : Grantee acocunt not valid");
            isValid = false;
            break;
          }
        }
      }
      // If any of the provided grantee is invalid then fail the request
      if (!isValid) {
        break;
      }
    }
    return isValid;
  }

  /**
   * Below method will validate the requested account
   *
   * @param grantee
   * @return true/false
   */
 protected
  boolean isGranteeValid(String grantee, ServerResponse response,
                         Map<String, List<Account>> accountPermissionMap,
                         String permission) {
    boolean isValid = true;
    AccountImpl accountImpl = new AccountImpl();
    String granteeDetails = grantee.substring(grantee.indexOf('=') + 1);
    Account account = null;
    try {
      if (grantee.contains("emailaddress=")) {
        account = accountImpl.findByEmailAddress(granteeDetails);
      } else if (grantee.contains("id=")) {
        account = accountImpl.findByCanonicalID(granteeDetails);
      }
      if (!account.exists()) {
        if (grantee.contains("emailaddress=")) {
          response = responseGenerator.unresolvableGrantByEmailAddress();
        } else if (grantee.contains("id=")) {
          response = responseGenerator.invalidArgument("Invalid id");
        }
        isValid = false;
      } else {  // update accountPermissionMap
        updateAccountPermissionMap(accountPermissionMap, permission, account);
      }
    }
    catch (DataAccessException e) {
      isValid = false;
    }
    return isValid;
  }

  /**
   * Updates the map if account exists
   *
   * @param accountPermissionMap
   * @param permission
   * @param account
   */
 private
  void updateAccountPermissionMap(
      Map<String, List<Account>> accountPermissionMap, String permission,
      Account account) {
    if (accountPermissionMap.get(permission) == null) {
      List<Account> accountList = new ArrayList<>();
      accountList.add(account);
      accountPermissionMap.put(permission, accountList);
    } else {
      accountPermissionMap.get(permission).add(account);
    }
  }
}

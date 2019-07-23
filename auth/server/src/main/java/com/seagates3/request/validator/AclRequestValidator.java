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

package com.seagates3.request.validator;

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

 public
  AclRequestValidator() {
    responseGenerator = new AuthorizationResponseGenerator();
    initializePermissionHeadersSet();
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
   * Below method will first check if only one of the below options is used
   * in request- 1. canned acl 2. permission header 3. acl in requestbody and
   * then method will perform requested account validation.
   *
   * @param requestBody
   * @return null- if request valid and errorResponse- if request not valid
   */
 public
  ServerResponse validateAclRequest(Map<String, String> requestBody) {
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
    if (response == null && (isPermissionHeaderPresent == 1)) {
      isValidPermissionHeader(requestBody, response);
    }
    return response;
  }

  /**
   * Below method will validate the requested account
   *
   * @param requestBody
   * @return
   */
 protected
  boolean isValidPermissionHeader(Map<String, String> requestBody,
                                  ServerResponse response) {
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
          if (!isGranteeValid(value, response)) {
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
  boolean isGranteeValid(String grantee, ServerResponse response) {
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
      }
    }
    catch (DataAccessException e) {
      isValid = false;
    }
    return isValid;
  }
}

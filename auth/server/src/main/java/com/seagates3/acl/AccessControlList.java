/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

/*
 * This Class represent Xml Node as
 <AccessControlList>
  <Grant>
   <Grantee xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:type="CanonicalUser">
    <ID>Grantee_ID</ID>
    <DisplayName>Grantee_Name</DisplayName>
   </Grantee>
   <Permission>Permission</Permission>
  </Grant>
 </AccessControlList>
 */

package com.seagates3.acl;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.xml.parsers.ParserConfigurationException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xml.sax.SAXException;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.ldap.GroupImpl;
import com.seagates3.exception.BadRequestException;
import com.seagates3.exception.DataAccessException;
import com.seagates3.exception.GrantListFullException;
import com.seagates3.model.Account;
import com.seagates3.util.BinaryUtil;

public
class AccessControlList {

 private
  ArrayList<Grant> grantList = new ArrayList<Grant>();
 private
  final Logger LOGGER =
      LoggerFactory.getLogger(AccessControlList.class.getName());

 public
  AccessControlList() {}

  ArrayList<Grant> getGrantList() { return new ArrayList<Grant>(grantList); }

  /**
   * Adds a grant to grantList if the list size is within permissible limits
   * @param grant
   * @throws GrantListFullException
   */
  void addGrant(Grant grant) throws GrantListFullException {

    if (grantList.size() < AuthServerConfig.MAX_GRANT_SIZE) {
      grantList.add(grant);
    } else {
      LOGGER.warn("Attempting to add Grant more than 100. Rejected");
      throw new GrantListFullException("Addition of Grant to ACL failed as " +
                                       "it reached maximum size");
    }
  }

  /**
   * Clear the grantList
   */
  void clearGrantList() { grantList.clear(); }

  void setGrant(Grant grant) {}

  /**
   * Get the grant for specified Canonical ID
   * @param CanonicalID
   * @return - {@link Grant}
   */
  List<Grant> getGrant(String CanonicalID) {
    List<Grant> matchingGrantList = new ArrayList<>();
    if (CanonicalID == null) return null;
    for (int counter = 0; counter < grantList.size(); counter++) {
      if (CanonicalID.equals(grantList.get(counter).grantee.canonicalId)) {
        matchingGrantList.add(grantList.get(counter));
      }
    }
    return matchingGrantList;
  }

  /**
   * Below method will check if permission availble for requested account
   *
   * @param account
   * @param requiredPermission
   * @param acp
   * @return
   * @throws DataAccessException
   */
 public
  boolean isPermissionAvailable(Account account, String requiredPermission,
                                String ownerId, boolean isUserAuthenticated,
                                String s3Action) throws DataAccessException {
    boolean isPermissionAvailable = false;
    if (account != null) {
      if (account.getCanonicalId().equals(ownerId)) {
        if (("READ_ACP".equals(requiredPermission) ||
             "WRITE_ACP".equals(requiredPermission))) {
        isPermissionAvailable = true;
        } else if (s3Action.equals("DeleteBucket")) {
          isPermissionAvailable = true;
        }
      }
    }

      if (!isPermissionAvailable) {
      for (int counter = 0; counter < this.getGrantList().size(); counter++) {
        Grant grantRecord = this.getGrantList().get(counter);
        GroupImpl groupImpl = new GroupImpl();
        String canonicalId = grantRecord.grantee.canonicalId;
        String emailId = grantRecord.grantee.emailAddress;
        String uri = grantRecord.grantee.uri;

        Grant grant = this.getGrantList().get(counter);

        if (account != null) {
          if ((canonicalId != null &&
               canonicalId.equals(account.getCanonicalId())) ||
              (emailId != null && emailId.equals(account.getEmail())) ||
              (uri != null &&
               groupImpl.findByPathAndAccount(account, uri).exists()) ||
              (isUserAuthenticated &&
               groupImpl.isPartOfAuthenticatedUsersGroup(uri)) ||
              (groupImpl.isPartOfAllUsersGroup(uri))) {
            if (hasPermission(grant, requiredPermission)) {
              isPermissionAvailable = true;
              break;
            }
          }

        } else if (groupImpl.isPartOfAllUsersGroup(uri)) {
          if (hasPermission(grant, requiredPermission)) {
            isPermissionAvailable = true;
            break;
          }
        }
      }
    }
    return isPermissionAvailable;
  }
  /**
       * Method actually checks expected and required permissions
       *
       * @param grant
       * @param requiredPermission
       * @param canonicalId
       * @param acp
       * @return
       */

  boolean hasPermission(Grant grant, String requiredPermission) {
    if (grant == null) return false;
    String permissionGrant = grant.getPermission();
    if ("FULL_CONTROL".equals(permissionGrant) ||
        requiredPermission.equals(permissionGrant)) {
      return true;
    }
    return false;
  }

  /**
      * Below will retrieve owner from bucket ACL
      * @param requestBody
      * @return
      */
 public
  String getOwner(Map<String, String> requestBody) {
    String encodedACL = requestBody.get("Auth-ACL");
    String owner = null;
    try {
      if (encodedACL == null || encodedACL.isEmpty()) {
        String ex = "Bad request. Resource ACL absent in the request.";
        LOGGER.error(ex);
        throw new BadRequestException(ex);
      }
      AccessControlPolicy acp =
          new AccessControlPolicy(BinaryUtil.base64DecodeString(encodedACL));
      owner = acp.getOwner().getCanonicalId();
    }
    catch (ParserConfigurationException | SAXException | IOException |
           GrantListFullException | BadRequestException e) {
      LOGGER.error("Exception while getting owner.. ", e);
    }

    return owner;
  }
}


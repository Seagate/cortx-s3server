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
 * Original author:  Abhilekh Mustapure <abhilekh.mustapure@seagate.com>
 * Original creation date: 04-Apr-2019
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
import java.util.ArrayList;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.GrantListFullException;

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
  Grant getGrant(String CanonicalID) {
    if (CanonicalID == null) return null;
    for (int counter = 0; counter < grantList.size(); counter++) {
      if (CanonicalID.equals(grantList.get(counter).grantee.canonicalId)) {
        return grantList.get(counter);
      }
    }

    return null;
  }
}

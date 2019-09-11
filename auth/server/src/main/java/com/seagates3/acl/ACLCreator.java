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
 * Original creation date: 08-Aug-2019
 */

package com.seagates3.acl;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xml.sax.SAXException;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.GrantListFullException;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.response.generator.AuthorizationResponseGenerator;
import com.seagates3.util.BinaryUtil;

public
class ACLCreator {

 private
  final Logger LOGGER = LoggerFactory.getLogger(ACLCreator.class.getName());
 protected
  static final Map<String, String> actualPermissionsMap = new HashMap<>();
  AuthorizationResponseGenerator responseGenerator =
      new AuthorizationResponseGenerator();
 private
  static String defaultACP = null;
 public
  ACLCreator() { initPermissionsMap(); }

 private
  void initPermissionsMap() {
    actualPermissionsMap.put("x-amz-grant-read", "READ");
    actualPermissionsMap.put("x-amz-grant-write", "WRITE");
    actualPermissionsMap.put("x-amz-grant-read-acp", "READ_ACP");
    actualPermissionsMap.put("x-amz-grant-write-acp", "WRITE_ACP");
    actualPermissionsMap.put("x-amz-grant-full-control", "FULL_CONTROL");
  }

  /**
   * Below method created default acl
   *
   * @param acp
   * @param requestor
   * @return
   * @throws SAXException
   * @throws ParserConfigurationException
   * @throws IOException
   * @throws GrantListFullException
   * @throws TransformerException
   */
 public
  String createDefaultAcl(Requestor requestor) throws IOException,
      ParserConfigurationException, SAXException, GrantListFullException,
      TransformerException {
    AccessControlPolicy acp =
        new AccessControlPolicy(checkAndCreateDefaultAcp());
    acp.initDefaultACL(requestor.getAccount().getCanonicalId(),
                       requestor.getAccount().getName());
    return acp.getXml();
  }

  /**
   * Below method updates existing grantee list with permission headers
   * requested
   *
   * @param acp
   * @param accountPermissionMap
   * @return
   * @throws GrantListFullException
   * @throws SAXException
   * @throws ParserConfigurationException
   * @throws IOException
   * @throws TransformerException
   */
 public
  String createAclFromPermissionHeaders(
      Requestor requestor, Map<String, List<Account>> accountPermissionMap,
      Map<String, String> requestBody) throws GrantListFullException,
      IOException, ParserConfigurationException, SAXException,
      TransformerException {
    AccessControlPolicy acp = null;
    AccessControlList newAcl = new AccessControlList();
    if (requestBody.get("ACL") != null) {
      acp = new AccessControlPolicy(
          BinaryUtil.base64DecodeString(requestBody.get("ACL")));
    } else {
      acp = new AccessControlPolicy(checkAndCreateDefaultAcp());
      }
      for (String permission : accountPermissionMap.keySet()) {
        for (Account account : accountPermissionMap.get(permission)) {
          Grantee grantee = new Grantee(account.getId(), account.getName());
          Grant grant =
              new Grant(grantee, actualPermissionsMap.get(permission));
          newAcl.addGrant(grant);
          LOGGER.info("Updated the acl for " + account.getName() +
                      " with permission - " + permission);
        }
      }
      if ("Owner_ID".equals(
              acp.getOwner().getCanonicalId())) {  // means acl getting created
                                                   // first time so set
                                                   // requestor as owner
        acp.setOwner(new Owner(requestor.getAccount().getCanonicalId(),
                               requestor.getAccount().getName()));
      }
      acp.setAccessControlList(newAcl);
      return acp.getXml();
    }

   protected
    String checkAndCreateDefaultAcp() throws IOException,
        ParserConfigurationException, SAXException, GrantListFullException {
      if (defaultACP == null) {
        defaultACP = new String(
            Files.readAllBytes(Paths.get(AuthServerConfig.authResourceDir +
                                         AuthServerConfig.DEFAULT_ACL_XML)));
      }
      return defaultACP;
    }
}


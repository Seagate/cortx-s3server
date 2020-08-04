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

package com.seagates3.acl;

import java.io.IOException;
import java.util.Map;

import javax.xml.parsers.ParserConfigurationException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xml.sax.SAXException;

import com.seagates3.exception.BadRequestException;
import com.seagates3.exception.DataAccessException;
import com.seagates3.exception.GrantListFullException;
import com.seagates3.model.Requestor;
import com.seagates3.util.ACLPermissionUtil;
import com.seagates3.util.BinaryUtil;

import io.netty.handler.codec.http.HttpMethod;

public
class ACLAuthorizer {

 private
  final Logger LOGGER = LoggerFactory.getLogger(ACLAuthorizer.class.getName());

  /**
   * Evaluates the request with the attached ACL and returns true if the
   * operation is authorized
   * @param requestor
   * @param requestBody
   * @return - true if Principal is authorized to perform requested operation
   * @throws ParserConfigurationException
   * @throws SAXException
   * @throws IOException
   * @throws BadRequestException
   * @throws GrantListFullException
   * @throws DataAccessException
   */
 public
  boolean isAuthorized(Requestor requestor, Map<String, String> requestBody)
      throws ParserConfigurationException,
      SAXException, IOException, BadRequestException, GrantListFullException,
      DataAccessException {

    String encodedACL = requestBody.get("Auth-ACL");
    if (encodedACL == null || encodedACL.isEmpty()) {
      String ex = "Bad request. Resource ACL absent in the request.";
      LOGGER.error(ex);
      throw new BadRequestException(ex);
    }

    AccessControlPolicy acp =
        new AccessControlPolicy(BinaryUtil.base64DecodeString(encodedACL));
    String method = requestBody.get("Method");
    if (method == null || method.isEmpty()) {
      String ex = "Invalid HTTP method: " + method;
      LOGGER.error(ex);
      throw new BadRequestException(ex);
    }
    HttpMethod httpMethod = HttpMethod.valueOf(method);

    String requiredPermission = ACLPermissionUtil.getACLPermission(
        httpMethod, requestBody.get("ClientAbsoluteUri"),
        requestBody.get("ClientQueryParams"));
    if (requiredPermission == null) {
      String ex = "Bad request. HTTP method or Permission unknown.";
      LOGGER.error(ex);
      throw new BadRequestException(ex);
    }
    if (requestor != null) {
      boolean isAuthorized = acp.getAccessControlList().isPermissionAvailable(
          requestor.getAccount(), requiredPermission,
          acp.getOwner().getCanonicalId(), true, requestBody.get("S3Action"));
      if (!isAuthorized) {
        LOGGER.debug("No Grants found in ACL for requested account");
        return false;
      }
      LOGGER.info("Request authorized");
      return true;
    } else {
      // AllUsers Group.
      boolean isAuthorized = acp.getAccessControlList().isPermissionAvailable(
          null, requiredPermission, acp.getOwner().getCanonicalId(), false, "");
      return isAuthorized;
    }
  }
}


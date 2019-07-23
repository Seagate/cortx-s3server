 /*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 27-May-2016
 */
package com.seagates3.authorization;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.BadRequestException;
import com.seagates3.exception.GrantListFullException;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.request.validator.AclRequestValidator;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Map;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xml.sax.SAXException;

public class Authorizer {

  private
   final Logger LOGGER = LoggerFactory.getLogger(Authorizer.class.getName());
  private
   static String defaultACP;

  public
   ServerResponse authorize(Requestor requestor,
                            Map<String, String> requestBody) {
        AuthorizationResponseGenerator responseGenerator
                = new AuthorizationResponseGenerator();
        LOGGER.debug("request body : " + requestBody.toString());
        ServerResponse serverResponse =
            new AclRequestValidator().validateAclRequest(requestBody);
        if (serverResponse != null) {
          LOGGER.error("ACL authorization request validation failed");
          return serverResponse;
        }
        LOGGER.info("ACL authorization request is validated successfully");
        // Authorize the request
        try {
          if (!new ACLAuthorizer().isAuthorized(requestor, requestBody)) {
            // TODO temporary comment till authserver receives ACL from
            // s3server
            // return responseGenerator.unauthorizedOperation();
          }
        }
        catch (ParserConfigurationException | SAXException | IOException |
               GrantListFullException e1) {
          LOGGER.error("Error while initializing ACP.");
          // TODO temporary comment till authserver receives ACL from
          // s3server
          // return responseGenerator.invalidACL();
        }
        catch (BadRequestException e2) {
          // TODO temporary comment till authserver receives ACL from
          // s3server
          // return responseGenerator.badRequest();
        }

        // Initialize a  default AccessControlPolicy object and generate
        // authorization response if request header contains param value true
        // for- Request-ACL
        if ("true".equals(requestBody.get("Request-ACL"))) {
          try {

            if (defaultACP == null) {
              defaultACP = new String(Files.readAllBytes(
                  Paths.get(AuthServerConfig.authResourceDir +
                            AuthServerConfig.DEFAULT_ACL_XML)));
            }
            AccessControlPolicy acp = new AccessControlPolicy(defaultACP);
            acp.initDefaultACL(requestor.getAccount().getCanonicalId(),
                               requestor.getAccount().getName());
            return responseGenerator.generateAuthorizationResponse(
                requestor, acp.getXml());
          }
          catch (ParserConfigurationException | SAXException | IOException e) {
            LOGGER.error("Error while initializing default ACL");
            return responseGenerator.invalidACL();
          }
          catch (GrantListFullException e) {
            LOGGER.error("Error while initializing default ACL." +
                         " Grants more than " +
                         AuthServerConfig.MAX_GRANT_SIZE + " are not allowed.");
            return responseGenerator.grantListSizeViolation();
          }
          catch (TransformerException e) {
            LOGGER.error("Error while generating the Authorization Response");
            return responseGenerator.internalServerError();
          }
        }

        // Validate the ACL if requested
        if (requestBody.get("Validate-ACL") != null) {
          ACLValidation aclValidation = null;
          try {
            aclValidation = new ACLValidation(requestBody.get("Validate-ACL"));
          }
          catch (ParserConfigurationException | SAXException | IOException e) {
            LOGGER.error("Error while Parsing ACLXML");
            return responseGenerator.invalidACL();
          }
          catch (GrantListFullException e) {
            LOGGER.error(
                "Error while Parsing ACLXML. Number of grants exceeds " +
                AuthServerConfig.MAX_GRANT_SIZE);
            return responseGenerator.invalidACL();
          }

          return aclValidation.validate();
        }
        return responseGenerator.generateAuthorizationResponse(requestor, null);
   }
   /**
      * Below method will check if user is root user or  no
      * @param user
      * @return
      */

  public
   static boolean isRootUser(User user) {
     boolean isRoot = false;
     if ("root".equals(user.getName())) {
       isRoot = true;
     }
     return isRoot;
   }
 }

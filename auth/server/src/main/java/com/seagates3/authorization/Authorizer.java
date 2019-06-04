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
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;
import java.io.File;
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

        /**
         * TODO - This is a temporary solution. Write the logic to authorize the
         * user.
         */
        File f = new File("/tmp/seagate_s3_user_unauthorized");
        if (f.exists())
            return responseGenerator.unauthorizedOperation();

        // Initialize a  default AccessControlPolicy object and generate
        // authorization response if request header contains param value true
        // for-
        // Request-Default-Object-ACL
        if ("true".equals(requestBody.get("Request-Object-ACL"))) {
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
            return responseGenerator.internalServerError();
          }
          catch (TransformerException e) {
            LOGGER.error("Error while generating the Authorization Response");
            return responseGenerator.internalServerError();
          }
        }
        return responseGenerator.generateAuthorizationResponse(requestor, null);
    }

}

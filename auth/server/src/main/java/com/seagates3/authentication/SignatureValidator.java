/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original creation date: 17-Sep-2014
 */
package com.seagates3.authentication;

import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthenticationResponseGenerator;
import com.seagates3.util.IEMUtil;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SignatureValidator {

    private final Logger LOGGER = LoggerFactory.getLogger(
            SignatureValidator.class.getName());

    private final String SIGNER_PACKAGE = "com.seagates3.authentication";

    public ServerResponse validate(ClientRequestToken clientRequestToken,
            Requestor requestor) {

        AuthenticationResponseGenerator responseGenerator
                = new AuthenticationResponseGenerator();

        AWSSign awsSign = getSigner(clientRequestToken);

        Boolean isRequestorAuthenticated = awsSign.authenticate(
                clientRequestToken, requestor);
        if (!isRequestorAuthenticated) {
            LOGGER.debug("Requestor is not authenticated.");
            return responseGenerator.signatureDoesNotMatch();
        }

        LOGGER.debug("Requestor is authenticated.");
        return responseGenerator.ok();
    }

    /*
     * <IEM_INLINE_DOCUMENTATION>
     *     <event_code>048004001</event_code>
     *     <application>S3 Authserver</application>
     *     <submodule>Reflection</submodule>
     *     <description>Failed to get required class</description>
     *     <audience>Development</audience>
     *     <details>
     *         Class not found exception occurred.
     *         The data section of the event has following keys:
     *           time - timestamp
     *           node - node name
     *           pid - process id of Authserver
     *           file - source code filename
     *           line - line number within file where error occurred
     *           cause - cause of exception
     *     </details>
     *     <service_actions>
     *         Save authserver log files and contact development team for
     *         further investigation.
     *     </service_actions>
     * </IEM_INLINE_DOCUMENTATION>
     *
     */
    /*
     * Get the client request version and return the AWS signer object.
     */
    private AWSSign getSigner(ClientRequestToken clientRequestToken) {
        LOGGER.debug("Signature version "
                + clientRequestToken.getSignVersion().toString());

        String signVersion = clientRequestToken.getSignVersion().toString();
        String signerClassName = toSignerClassName(signVersion);

        Class<?> awsSigner;
        Object obj;

        try {
            awsSigner = Class.forName(signerClassName);
            obj = awsSigner.newInstance();

            return (AWSSign) obj;
        } catch (ClassNotFoundException | SecurityException ex) {
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.CLASS_NOT_FOUND_EX,
                    "Failed to get required class",
                    String.format("\"cause\": \"%s\"", ex.getCause()));
        } catch (IllegalAccessException | IllegalArgumentException | InstantiationException ex) {
            LOGGER.error("Exception: ", ex);
        }

        return null;
    }

    /*
     * Return the class name of the AWS signer.
     */
    private String toSignerClassName(String signVersion) {
        return String.format("%s.AWS%sSign", SIGNER_PACKAGE, signVersion);
    }
}

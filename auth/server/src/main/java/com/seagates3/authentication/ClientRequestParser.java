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
 * Original creation date: 22-Oct-2015
 */
package com.seagates3.authentication;

import com.seagates3.exception.InvalidTokenException;
import com.seagates3.util.IEMUtil;
import io.netty.handler.codec.http.FullHttpRequest;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ClientRequestParser {

    private static final String REQUEST_PARSER_PACKAGE
            = "com.seagates3.authentication";
    private static final String AWS_V2_AUTHRORIAZATION_PATTERN
            = "AWS [A-Za-z0-9-_]+:[a-zA-Z0-9+/=]+";
    private static final String AWS_V4_AUTHRORIAZATION_PATTERN
            = "AWS4-HMAC-SHA256[\\w\\W]+";
    private final static Logger LOGGER =
            LoggerFactory.getLogger(ClientRequestParser.class.getName());


    public static ClientRequestToken parse(FullHttpRequest httpRequest,
            Map<String, String> requestBody) {
        String requestAction = requestBody.get("Action");
        String authorizationHeader;
        ClientRequestToken.AWSSigningVersion awsSigningVersion;

        if (requestAction.equals("AuthenticateUser")
                || requestAction.equals("AuthorizeUser")) {
            authorizationHeader = requestBody.get("authorization");
        } else {
            authorizationHeader = httpRequest.headers().get("authorization");
        }

        if (authorizationHeader == null) {
            return null;
        }

        if (authorizationHeader.matches(AWS_V2_AUTHRORIAZATION_PATTERN)) {
            awsSigningVersion = ClientRequestToken.AWSSigningVersion.V2;
        }
        else if(authorizationHeader.matches(AWS_V4_AUTHRORIAZATION_PATTERN))
        {
            awsSigningVersion = ClientRequestToken.AWSSigningVersion.V4;
        }
        else
        {
            return null;
        }

        AWSRequestParser awsRequestParser = getAWSRequestParser(awsSigningVersion);
        ClientRequestToken clientrequesttoken = null;
        if (requestAction.equals("AuthenticateUser")
                || requestAction.equals("AuthorizeUser")) {
            try {
                clientrequesttoken = awsRequestParser.parse(requestBody);
            } catch (InvalidTokenException ex) {
                LOGGER.error("Error while parsing request : "+ ex.getMessage());
            }
        } else {

            try {
                clientrequesttoken = awsRequestParser.parse(httpRequest);
            } catch (InvalidTokenException ex) {
                    LOGGER.error("Error while parsing request : "+ ex.getMessage());
            }
        }
        return clientrequesttoken;
    }

    public static AWSRequestParser getAWSRequestParser(
            ClientRequestToken.AWSSigningVersion awsSigningVersion) {

        String signVersion = awsSigningVersion.toString();
        String awsRequestParserClass = toRequestParserClass(signVersion);

        Class<?> awsRequestParser;
        Object obj;

        try {
            awsRequestParser = Class.forName(awsRequestParserClass);
            obj = awsRequestParser.newInstance();

            return (AWSRequestParser) obj;
        } catch (ClassNotFoundException | SecurityException ex) {
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.CLASS_NOT_FOUND_EX,
                    "Failed to get required class",
                    String.format("\"cause\": \"%s\"", ex.getCause()));
        } catch (IllegalAccessException | IllegalArgumentException | InstantiationException ex) {
            System.out.println(ex);
        }

        return null;
    }

    private static String toRequestParserClass(String signingVersion) {
        return String.format("%s.AWSRequestParser%s", REQUEST_PARSER_PACKAGE, signingVersion);
    }
}

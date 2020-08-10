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

package com.seagates3.s3service;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Arrays;

import org.apache.commons.httpclient.HttpStatus;
import org.apache.http.HttpResponse;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.S3RequestInitializationException;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthenticationResponseGenerator;


public class S3AccountNotifier {
    private final Logger LOGGER = LoggerFactory.getLogger(S3AccountNotifier.class.getName());
    public AuthenticationResponseGenerator responseGenerator =
                                                 new AuthenticationResponseGenerator();

    private String getEndpointURL() {
        String s3EndPoint = AuthServerConfig.getDefaultEndpoint();
        String s3ConnectMode = "http";
        if (AuthServerConfig.isEnableHttpsToS3()) {
        	s3ConnectMode = "https";
        };
        //s3ConnectMode will have value http or https
        String endPoint = s3ConnectMode + "://" + s3EndPoint;
        return endPoint;
    }

    /**
     * Sends New Account notification to S3 Server
     * @param accountId
     * @param accessKey
     * @param secretKey
     * @return
     */
    public ServerResponse notifyNewAccount(String accountId, String accessKey,
                                                           String secretKey) {

        S3RestClient s3Client = new S3RestClient();

        String resourceUrl = "/account/" + accountId;
        s3Client.setResource(resourceUrl);
        s3Client.setCredentials(accessKey, secretKey);

        s3Client.setURL(getEndpointURL() + resourceUrl);

        //Set S3 Management API header
        s3Client.setHeader("x-seagate-mgmt-api", "true");

        S3HttpResponse resp = null;
        try {
            resp = s3Client.postRequest();
        } catch (S3RequestInitializationException e) {
            LOGGER.error("S3 REST Request initialization failed with"
                    + "error message:" + e.getMessage());
            return responseGenerator.internalServerError();
        } catch (IOException e) {
            StringWriter stack = new StringWriter();
            e.printStackTrace(new PrintWriter(stack));
            LOGGER.error("S3 REST Request failed error stack:"
                    + stack.toString());
            return responseGenerator.internalServerError();
        }

        int httpCode = resp.getHttpCode();
        if(httpCode == HttpStatus.SC_CREATED) {
            LOGGER.info("New Account [" + accountId +"] create notification "
                    + "to S3 sent successfully");
            return responseGenerator.ok();
        } else {
            LOGGER.error("New Account [" + accountId +"] create notification"
                    + " to S3 failed with httpCode:" + httpCode);
            return responseGenerator.internalServerError();
        }

    }

    /**
     * Sends Delete notification to S3 Server
     * @param accountId
     * @param accessKey
     * @param secretKey
     * @return ServerResponse
     */
   public
    ServerResponse notifyDeleteAccount(String accountId, String accessKey,
                                       String secretKey, String securityToken) {
        S3RestClient s3Client = new S3RestClient();

        String resourceUrl = "/account/" + accountId;

        s3Client.setResource(resourceUrl);

        s3Client.setCredentials(accessKey, secretKey);
        s3Client.setURL(getEndpointURL() + resourceUrl);
        //Set S3 Management API header
        s3Client.setHeader("x-seagate-mgmt-api", "true");
        if (securityToken != null) {
          s3Client.setHeader("x-amz-security-token", securityToken);
        }
        S3HttpResponse resp = null;
        try {
            resp = s3Client.deleteRequest();
        } catch (S3RequestInitializationException e) {
            LOGGER.error("S3 REST Request initialization failed with"
                                  + "error message:" + e.getMessage());
            return responseGenerator.internalServerError();
        } catch (IOException e) {
            StringWriter stack = new StringWriter();
            e.printStackTrace(new PrintWriter(stack));
            LOGGER.error("S3 REST Request failed error stack:"
                    + stack.toString() );
            return responseGenerator.internalServerError();
        }

        int httpCode = resp.getHttpCode();
        if( httpCode == HttpStatus.SC_CONFLICT) {
            LOGGER.error("Account [" + accountId +"] is not empty, cannot"
                    + " delete account. httpCode:" + httpCode);
            return responseGenerator.accountNotEmpty();
        }

        LOGGER.info("Account [" + accountId +"] delete notification "
                + "to S3 sent successfully");
        if (httpCode == HttpStatus.SC_NO_CONTENT) {
            LOGGER.info("Account [" + accountId + "] does not own any "
                    + "resources, safe to delete");
            return responseGenerator.ok();
        }
        LOGGER.error("Account [" + accountId +"] delete notification to S3"
                + "failed with httpCode:" + httpCode);
        return responseGenerator.internalServerError();
    }

}


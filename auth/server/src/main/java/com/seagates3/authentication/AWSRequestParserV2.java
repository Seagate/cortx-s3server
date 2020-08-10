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

package com.seagates3.authentication;

import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpHeaders;
import java.util.Map;
import java.util.Arrays;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.exception.InvalidTokenException;

public class AWSRequestParserV2 extends AWSRequestParser {

    private final Logger LOGGER =
            LoggerFactory.getLogger(AWSRequestParserV2.class.getName());

    @Override
    public ClientRequestToken parse(FullHttpRequest httpRequest) throws InvalidTokenException {
        ClientRequestToken clientRequestToken = new ClientRequestToken();
        clientRequestToken.setSignedVersion(ClientRequestToken.AWSSigningVersion.V2);

        parseRequestHeaders(httpRequest, clientRequestToken);

        HttpHeaders httpHeaders = httpRequest.headers();
        authHeaderParser(httpHeaders.get("authorization"), clientRequestToken);

        return clientRequestToken;
    }

    @Override
    public ClientRequestToken parse(Map<String, String> requestBody) throws InvalidTokenException {
        ClientRequestToken clientRequestToken = new ClientRequestToken();
        clientRequestToken.setSignedVersion(ClientRequestToken.AWSSigningVersion.V2);

        parseRequestHeaders(requestBody, clientRequestToken);

        authHeaderParser(requestBody.get("Authorization"), clientRequestToken);

        return clientRequestToken;
    }

    /*
     * Authorization header is in following format
     * Authorization: "AWS" + " " + AWSAccessKeyId + ":" + Signature
     *
     * Sample Authorization Header
     * Authorization: AWS AKIAIOSFODNN7EXAMPLE:MyyxeRY7whkBe+bq8fHCL/2kKUg=
     */
    @Override
    public void authHeaderParser(String authorizationHeaderValue,
            ClientRequestToken clientRequestToken) throws InvalidTokenException {
        String[] tokens, subTokens;
        String errMsg = "Invalid client request token found while parsing request";

        tokens = authorizationHeaderValue.split(" ");
        if (tokens.length < 2) {
            LOGGER.error("Found "+ tokens.length + " tokens "+ Arrays.toString(tokens) +
                       " while parsing but required atleast 2 subtokens.");
            throw new InvalidTokenException(errMsg);
        }
        subTokens = tokens[1].split(":");
        if (subTokens.length < 2) {
            LOGGER.error("Found "+ subTokens.length + " subtokens "+ Arrays.toString(subTokens) +
                       " while parsing but required atleast 2 subtokens.");
            throw new InvalidTokenException(errMsg);
        }

        clientRequestToken.setAccessKeyId(subTokens[0]);
        clientRequestToken.setSignature(subTokens[1]);
    }
}

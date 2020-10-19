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

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.InvalidTokenException;

import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpHeaders;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public abstract class AWSRequestParser {

    /**
     * Parse the client request.
     *
     * @param httpRequest
     * @return
     * @throws InvalidTokenException
     */
    public abstract ClientRequestToken parse(FullHttpRequest httpRequest)
            throws InvalidTokenException;

    /**
     *
     * @param requestBody
     * @return
     * @throws InvalidTokenException
     */
    public abstract ClientRequestToken parse(Map<String, String> requestBody)
            throws InvalidTokenException;

    /**
     * Parse the authorization header.
     *
     * @param authorizationHeaderValue
     * @param clientRequestToken
     * @throws InvalidTokenException
     */
    protected abstract void authHeaderParser(String authorizationHeaderValue,
            ClientRequestToken clientRequestToken) throws InvalidTokenException;

    /**
     * Parse the request headers.
     *
     * @param httpRequest
     * @param clientRequestToken
     */
    protected void parseRequestHeaders(FullHttpRequest httpRequest,
            ClientRequestToken clientRequestToken) {
        HttpHeaders httpHeaders = httpRequest.headers();
        Map<String, String> requestHeaders
                = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);

        for (Entry<String, String> e : httpHeaders) {
            requestHeaders.put(e.getKey(), e.getValue());
        }

        clientRequestToken.setHttpMethod(httpRequest.method().toString());

        String host = httpHeaders.get("host");
        parseHostHeader(clientRequestToken, host);

        String[] tokens = httpRequest.uri().split("\\?", -1);
        clientRequestToken.setUri(tokens[0]);

        if (tokens.length == 2) {
            clientRequestToken.setQuery(tokens[1]);
        } else {
            clientRequestToken.setQuery("");
        }

        clientRequestToken.setRequestHeaders(requestHeaders);
    }

    /**
     * Parse the client request.
     *
     * @param requestBody
     * @param clientRequestToken
     */
    protected void parseRequestHeaders(Map<String, String> requestBody,
            ClientRequestToken clientRequestToken) {

        clientRequestToken.setHttpMethod(requestBody.get("Method"));

        String host = requestBody.get("host");
        parseHostHeader(clientRequestToken, host);

        clientRequestToken.setUri(requestBody.get("ClientAbsoluteUri"));
        clientRequestToken.setQuery(requestBody.get("ClientQueryParams"));

        clientRequestToken.setRequestHeaders(requestBody);
    }

    /*
     * Parse host header, and identify if request is virtual hosted-style and
     * set bucket name if it is.
     */
    private void parseHostHeader(ClientRequestToken clientRequestToken, String host) {
        List<String> uriEndpoints = new ArrayList<>();
        List<String> s3Endpoints = Arrays.asList(AuthServerConfig.getEndpoints());
        String patternString;
        Pattern pattern;
        Matcher matcher;

        uriEndpoints.add(AuthServerConfig.getDefaultEndpoint());
        uriEndpoints.addAll(s3Endpoints);

        for (String endPoint : uriEndpoints) {
            patternString = String.format("(^.*)(\\.)(%s)([\\d:]*$)", endPoint);
            pattern = Pattern.compile(patternString);
            matcher = pattern.matcher(host);
            if (matcher.matches()) {
                clientRequestToken.setVirtualHost(Boolean.TRUE);
                clientRequestToken.setBucketName(matcher.group(1));
                return;
            }
        }
        clientRequestToken.setVirtualHost(Boolean.FALSE);
    }
}

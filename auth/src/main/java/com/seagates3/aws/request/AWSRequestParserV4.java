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

package com.seagates3.aws.request;

import java.util.Map;

import io.netty.handler.codec.http.HttpHeaders;
import io.netty.handler.codec.http.HttpRequest;

import com.seagates3.model.ClientRequestToken;

public class AWSRequestParserV4 extends AWSRequestParser {

    @Override
    public ClientRequestToken parse(HttpRequest httpRequest) {
        ClientRequestToken clientRequestToken = new ClientRequestToken();
        clientRequestToken.setSignedVersion(ClientRequestToken.AWSSigningVersion.V4);

        parseRequestHeaders(httpRequest, clientRequestToken);

        HttpHeaders httpHeaders = httpRequest.headers();
        authHeaderParser(httpHeaders.get("authorization"), clientRequestToken);
        createCanonicalHeader(httpHeaders, clientRequestToken);

        return clientRequestToken;
    }

    @Override
    public ClientRequestToken parse(Map<String, String> requestBody) {
        ClientRequestToken clientRequestToken = new ClientRequestToken();
        clientRequestToken.setSignedVersion(ClientRequestToken.AWSSigningVersion.V4);

        parseRequestHeaders(requestBody, clientRequestToken);

        authHeaderParser(requestBody.get("Authorization"), clientRequestToken);
        createCanonicalHeader(requestBody, clientRequestToken);

        return clientRequestToken;
    }

    /*
     * Authorization header is in following format (Line break added for readability)
     * Authorization: algorithm Credential=access key ID/credential scope,
     * SignedHeaders=SignedHeaders, Signature=signature
     *
     * Sample Authorization Header
     * Authorization:AWS4-HMAC-SHA256 Credential=AKIAJTYX36YCKQSAJT7Q/20150810/us-east-1/iam/aws4_request,
     * SignedHeaders=content-type;host;user-agent;x-amz-content-sha256;x-amz-date,
     * Signature=b751427a69f5bb76fb171fec45bdb1e8f664fac7f7c23c983f9c7361bb382d76
     *
     * Authorization header value is broken into 3 chunks.
     * tokens[0] : algorithm Credential=access key ID/credential scope
     * tokens[1] : SignedHeaders=SignedHeaders
     * tokens[2] : Signature=signature
     *
     * Credential Scope = date/region/service/aws4_request
     */
    @Override
    public void authHeaderParser(String authorizationHeaderValue,
            ClientRequestToken clientRequestToken) {
        String[] tokens = authorizationHeaderValue.split(",");
        String[] subTokens, credTokens, credScopeTokens;

        subTokens = tokens[0].split(" ");
        clientRequestToken.setSigningAlgorithm(subTokens[0]);

        credTokens = subTokens[1].split("/", 2);
        clientRequestToken.setAccessKeyId(credTokens[0].split("=")[1]);

        String credentialScope = credTokens[1];
        clientRequestToken.setCredentialScope(credentialScope);

        credScopeTokens = credentialScope.split("/");
        clientRequestToken.setDate(credScopeTokens[0]);
        clientRequestToken.setRegion(credScopeTokens[1]);
        clientRequestToken.setService(credScopeTokens[2]);
        //signVersion = credScopeTokens[3];

        subTokens = tokens[1].split("=");
        clientRequestToken.setSignedHeaders(subTokens[1]);
        clientRequestToken.setSignature(tokens[2].split("=")[1]);
    }

    /*
     * The canonical headers consist of a list of all the HTTP headers are
     * include as a part of the AWS request.
     * Convert all header names to lowercase and trim excess white space
     * characters out of the header values.
     *
     * Canonical Header = CanonicalHeadersEntry0 + CanonicalHeadersEntry1
     *                      + ... + CanonicalHeadersEntryN
     *
     * CanonicalHeadersEntry = Lowercase(HeaderName) + ':' + Trimall(HeaderValue) + '\n'
     */
    private void createCanonicalHeader(HttpHeaders httpHeaders,
            ClientRequestToken clientRequestToken) {
        String headerValue;
        String canonicalHeader = "";

        for(String s : clientRequestToken.getSignedHeaders().split(";")) {
            if(s.equalsIgnoreCase("content-type")) {
                headerValue = httpHeaders.get(s);

                /*
                 * Strangely, the aws .net sdk doesn't send the content type.
                 * Hence the content type is hard coded.
                 */
                if(headerValue.isEmpty()) {
                    canonicalHeader += "content-type:application/x-www-form-urlencoded;"
                                    + " charset=utf-8\n";
                } else {
                    canonicalHeader += String.format("%s:%s\n", s, headerValue);
                }
            } else {
                headerValue = httpHeaders.get(s);
                canonicalHeader += String.format("%s:%s\n", s, headerValue);
            }
        }

        clientRequestToken.setCanonicalHeader(canonicalHeader);
    }

    private void createCanonicalHeader(Map<String, String> requestBody,
            ClientRequestToken clientRequestToken) {
        String headerValue;
        String canonicalHeader = "";

        for(String s : clientRequestToken.getSignedHeaders().split(";")) {
            if(s.equalsIgnoreCase("content-type")) {
                headerValue = requestBody.get(s);

                /*
                 * Strangely, the aws .net sdk doesn't send the content type.
                 * Hence the content type is hard coded.
                 */
                if(headerValue.isEmpty()) {
                    canonicalHeader += "content-type:application/x-www-form-urlencoded;"
                                    + " charset=utf-8\n";
                } else {
                    canonicalHeader += String.format("%s:%s\n", s, headerValue);
                }
            } else {
                headerValue = requestBody.get(s);
                canonicalHeader += String.format("%s:%s\n", s, headerValue);
            }
        }

        clientRequestToken.setCanonicalHeader(canonicalHeader);
    }
}

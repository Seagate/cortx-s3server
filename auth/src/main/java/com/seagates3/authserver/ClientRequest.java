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

package com.seagates3.authserver;

import java.util.Map;

import io.netty.handler.codec.http.HttpHeaders;
import io.netty.handler.codec.http.HttpRequest;

/**
 *
 * @author Arjun Hariharan
 */

public class ClientRequest {
    String algorithm;
    String accessKeyId;
    String credentialScope;
    String signature;
    String region;
    String service;
    String date;
    String signVersion;
    String signedHeaders;
    String httpMethod;
    String canonicalQuery;
    String canonicalUri;
    String sessionToken;
    String hashedPayLoad;
    String canonicalHeader;
    String requestDate;

    public ClientRequest(HttpRequest httpRequest) {
        HttpHeaders httpHeaders = httpRequest.headers();

        this.sessionToken = httpHeaders.get("x-amz-security-token");
        this.httpMethod = httpRequest.getMethod().toString();

        String []tokens = httpRequest.getUri().split("\\?", -1);
        this.canonicalUri = tokens[0];

        if(tokens.length == 2) {
            this.canonicalQuery = tokens[1];
        } else {
            this.canonicalQuery = "";
        }

        this.hashedPayLoad = httpHeaders.get("x-amz-content-sha256");
        this.requestDate = httpHeaders.get("x-amz-date");

        authHeaderParser(httpHeaders.get("authorization"));
        createCanonicalHeader(httpHeaders);
    }

    public ClientRequest(Map<String, String> requestBody) {
        this.sessionToken = requestBody.get("x-amz-security-token");
        this.httpMethod = requestBody.get("Method");
        this.canonicalUri = requestBody.get("CanonicalUri");
        this.canonicalQuery = requestBody.get("CanonicalQuery");
        this.hashedPayLoad = requestBody.get("x-amz-content-sha256");
        this.requestDate = requestBody.get("x-amz-date");

        authHeaderParser(requestBody.get("Authorization"));
        createCanonicalHeader(requestBody);
    }

    /*
     * Return the HTTP method.
     */
    public String getMethod() {
        return httpMethod;
    }

    /*
     * Return the canonical URI
     */
    public String getCanonicalUri() {
        return canonicalUri;
    }

    /*
     * Return the canonical query
     */
    public String getCanonicalQuery() {
        return canonicalQuery;
    }

    /*
     * Return the Algorithm.
     */
    public String getAlgorithm() {
        return algorithm;
    }

    /*
     * Return the Access Key Id of the requestor.
     */
    public String getAccessKeyId() {
        return accessKeyId;
    }

    /*
     * Return the credential scope
     */
    public String getCredentialScope() {
        return credentialScope;
    }

    /*
     * Return the request signature.
     */
    public String getSignature() {
        return signature;
    }

    /*
     * Return the target region.
     */
    public String getRegion() {
        return region;
    }

    /*
     * Return the requested service name.
     */
    public String getService() {
        return service;
    }

    /*
     * Return the signed header array.
     */
    public String getSignedHeaders() {
        return signedHeaders;
    }

    /*
     * Return the date.
     */
    public String getDate() {
        return date;
    }

    public String getSignVersion() {
        return signVersion;
    }

    /*
     * Return the hashed pay load.
     */
    public String getHashedPayLoad() {
        return hashedPayLoad;
    }

    /*
     * Return the canonical header.
     */
    public String getCanonicalHeader() {
        return canonicalHeader;
    }

    /*
     * Return the request date.
     */
    public String getRequestDate() {
        return requestDate;
    }

    /*
     * Return the session token.
     */
    public String getSessionToken() {
        return sessionToken;
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
    private void authHeaderParser(String authorizationHeaderValue) {
        String[] tokens = authorizationHeaderValue.split(",");
        String[] subTokens, credTokens, credScopeTokens;

        subTokens = tokens[0].split(" ");
        algorithm = subTokens[0];

        credTokens = subTokens[1].split("/", 2);
        accessKeyId = credTokens[0].split("=")[1];
        credentialScope = credTokens[1];

        credScopeTokens = credentialScope.split("/");
        date = credScopeTokens[0];
        region = credScopeTokens[1];
        service = credScopeTokens[2];
        signVersion = credScopeTokens[3];

        subTokens = tokens[1].split("=");
        signedHeaders = subTokens[1];

        signature = tokens[2].split("=")[1];
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
    private void createCanonicalHeader(HttpHeaders httpHeaders) {
        String headerValue;
        canonicalHeader = "";

        for(String s : signedHeaders.split(";")) {
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
    }

    private void createCanonicalHeader(Map<String, String> requestBody) {
        String headerValue;
        canonicalHeader = "";

        for(String s : signedHeaders.split(";")) {
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
    }
}

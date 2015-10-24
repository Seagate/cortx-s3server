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

package com.seagates3.model;

public class ClientRequestToken {
    public enum AWSSigningVersion {
        V4, V2;
    }

    String accessKeyId;
    String canonicalHeader;
    String canonicalQuery;
    /*
     * Canonical URI = URI - Bucket Name
     * Ex -
     * Suppose URI = /bucket/myfile
     *
     * Canonical URI = /myfile
     *
     */
    String canonicalUri;
    String credentialScope;
    String date;
    String hashedPayLoad;
    String httpMethod;
    String region;
    String requestDate;
    String service;
    String sessionToken;
    String signature;
    String signedHeaders;
    String signingAlgorithm;
    AWSSigningVersion signVersion;

    String uri;


    /*
     * Return the Access Key Id of the requestor.
     */
    public String getAccessKeyId() {
        return accessKeyId;
    }

    /*
     * Return the canonical header.
     */
    public String getCanonicalHeader() {
        return canonicalHeader;
    }

    /*
     * Return the canonical query
     */
    public String getCanonicalQuery() {
        return canonicalQuery;
    }

    /*
     * Return the canonical URI.
     */
    public String getCanonicalUri() {
        return canonicalUri;
    }

    /*
     * Return the credential scope
     */
    public String getCredentialScope() {
        return credentialScope;
    }

    /*
     * Return the date.
     */
    public String getDate() {
        return date;
    }

    /*
     * Return the hashed pay load.
     */
    public String getHashedPayLoad() {
        return hashedPayLoad;
    }

    /*
     * Return the HTTP method.
     */
    public String getHttpMethod() {
        return httpMethod;
    }

    /*
     * Return the target region.
     */
    public String getRegion() {
        return region;
    }

    /*
     * Return the request date.
     */
    public String getRequestDate() {
        return requestDate;
    }

    /*
     * Return the requested service name.
     */
    public String getService() {
        return service;
    }

    /*
     * Return the session token.
     */
    public String getSessionToken() {
        return sessionToken;
    }

    /*
     * Return the request signature.
     */
    public String getSignature() {
        return signature;
    }

    /*
     * Return the signed header array.
     */
    public String getSignedHeaders() {
        return signedHeaders;
    }

    /*
     * Return the signing algorithm.
     */
    public String getSigningAlgorithm() {
        return signingAlgorithm;
    }

    /*
     * Return the signature version
     */
    public AWSSigningVersion getSignVersion() {
        return signVersion;
    }

    /*
     * Return the uri.
     */
    public String getUri() {
        return uri;
    }

    /*
     * Set the access key id.
     */
    public void setAccessKeyId(String accessKeyId) {
        this.accessKeyId = accessKeyId;
    }

    /*
     * Set the canonical header.
     */
    public void setCanonicalHeader(String canonicalHeader) {
        this.canonicalHeader = canonicalHeader;
    }

    /*
     * Set the canonical Query.
     */
    public void setCanonicalQuery(String canonicalQuery) {
        this.canonicalQuery = canonicalQuery;
    }

    /*
     * Set the canonical Uri.
     */
    public void setCanonicalUri(String canonicalUri) {
        this.canonicalUri = canonicalUri;
    }

    /*
     * Set the credential scope.
     */
    public void setCredentialScope(String credentialScope) {
        this.credentialScope = credentialScope;
    }

    /*
     * Set the Date.
     */
    public void setDate(String date) {
        this.date = date;
    }

    /*
     * Set the hashed payload.
     */
    public void setHashedPayload(String hashedPayLoad) {
        this.hashedPayLoad = hashedPayLoad;
    }

    /*
     * Set the http method.
     */
    public void setHttpMethod(String httpMethod) {
        this.httpMethod = httpMethod;
    }

    /*
     * Set the region.
     */
    public void setRegion(String region) {
        this.region = region;
    }

    /*
     * Set the request date.
     */
    public void setRequestDate(String requestDate) {
        this.requestDate = requestDate;
    }

    /*
     * Set the service name.
     */
    public void setService(String service) {
        this.service = service;
    }

    /*
     * Set the session token.
     */
    public void setSessionToken(String sessionToken) {
        this.sessionToken = sessionToken;
    }

    /*
     * Set the signature
     */
    public void setSignature(String signature) {
        this.signature = signature;
    }

    /*
     * Set the signed headers.
     */
    public void setSignedHeaders(String signedHeaders) {
        this.signedHeaders = signedHeaders;
    }

    /*
     * Set the siginig algorithm.
     */
    public void setSigningAlgorithm(String algorithm) {
        this.signingAlgorithm = algorithm;
    }

    /*
     * Set the signed version.
     */
    public void setSignedVersion(AWSSigningVersion signVersion) {
        this.signVersion = signVersion;
    }

    /*
     * Set the uri.
     */
    public void setUri(String uri) {
        this.uri = uri;
    }
}

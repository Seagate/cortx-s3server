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
import com.seagates3.util.BinaryUtil;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.URLEncoder;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AWSV4Sign implements AWSSign {

    private final Logger LOGGER = LoggerFactory.getLogger(
            AWSV4Sign.class.getName());

    private final String STREAMING_AWS4_HMAC_SHA256_PAYLOAD
            = "STREAMING-AWS4-HMAC-SHA256-PAYLOAD";

    /**
     * Return true if the signature is valid.
     *
     * @param clientRequestToken
     * @param requestor
     * @return True if authenticated.
     */
    @Override
    public Boolean authenticate(ClientRequestToken clientRequestToken,
            Requestor requestor) {
        Map<String, String> requestHeaders
                = clientRequestToken.getRequestHeaders();

        if (requestHeaders.containsKey("x-amz-content-sha256")
                && requestHeaders.get("x-amz-content-sha256")
                .equals(STREAMING_AWS4_HMAC_SHA256_PAYLOAD)) {

            LOGGER.debug("Chunked upload. Verify seed signature");
            return verifyChunkedSeedSignature(clientRequestToken, requestor);
        } else if (requestHeaders.containsKey("previous-signature-sha256")) {

            LOGGER.debug("Chunked upload. Verify chunk signature.");
            return verifyChunkedSignature(clientRequestToken, requestor);
        } else {

            LOGGER.debug("Verify signature of unchunked request.");
            return verifySignature(clientRequestToken, requestor);
        }
    }

    /**
     * Verify the signature of a regular S3/IAM API.
     *
     * @param clientRequestToken
     * @param requestor
     * @return
     */
    private Boolean verifySignature(
            ClientRequestToken clientRequestToken, Requestor requestor) {
        String canonicalRequest, stringToSign, signature;
        byte[] signingKey;

        canonicalRequest = createCanonicalRequest(clientRequestToken);
        LOGGER.debug("Canonical request- " + canonicalRequest);

        stringToSign = createStringToSign(canonicalRequest, clientRequestToken);
        LOGGER.debug("String to sign- " + stringToSign);

        String secretKey = requestor.getAccesskey().getSecretKey();
        signingKey = deriveSigningKey(clientRequestToken, secretKey);
        LOGGER.debug("Signing key- " + signingKey);

        signature = calculateSignature(signingKey, stringToSign);
        LOGGER.debug("Request signature- " + clientRequestToken.getSignature());
        LOGGER.debug("Calculated signature- " + signature);

        return signature.equals(clientRequestToken.getSignature());
    }

    /**
     * Authenticate seed signature of a chunked upload request.
     *
     * @param clientRequestToken
     * @param requestor
     * @return
     */
    private Boolean verifyChunkedSeedSignature(
            ClientRequestToken clientRequestToken, Requestor requestor) {
        String canonicalRequest, stringToSign, signature;
        byte[] signingKey;

        canonicalRequest = createCanonicalRequestChunkedSeed(clientRequestToken);
        LOGGER.debug("Canonical request- " + canonicalRequest);

        stringToSign = createStringToSign(canonicalRequest,
                clientRequestToken);
        LOGGER.debug("String to sign- " + stringToSign);

        String secretKey = requestor.getAccesskey().getSecretKey();
        signingKey = deriveSigningKey(clientRequestToken, secretKey);
        LOGGER.debug("Signing key- " + signingKey);

        signature = calculateSignature(signingKey, stringToSign);
        LOGGER.debug("Request signature- " + clientRequestToken.getSignature());
        LOGGER.debug("Calculated signature- " + signature);

        return signature.equals(clientRequestToken.getSignature());
    }

    /**
     * Verify signature the chunk in a chunked upload request.
     *
     * @param clientRequestToken
     * @param requestor
     * @return
     */
    private Boolean verifyChunkedSignature(
            ClientRequestToken clientRequestToken, Requestor requestor) {
        String stringToSign, signature, currentSign;
        byte[] signingKey;

        stringToSign = createStringToSignChunked(clientRequestToken);
        LOGGER.debug("String to sign- " + stringToSign);

        String secretKey = requestor.getAccesskey().getSecretKey();
        signingKey = deriveSigningKey(clientRequestToken, secretKey);
        LOGGER.debug("Signing key- " + signingKey);

        signature = calculateSignature(signingKey, stringToSign);
        LOGGER.debug("Request signature- " + clientRequestToken.getSignature());
        LOGGER.debug("Calculated signature- " + signature);

        currentSign = clientRequestToken.getRequestHeaders()
                .get("current-signature-sha256");

        clientRequestToken.setSignature(currentSign);
        return signature.equals(currentSign);
    }

    /**
     * Canonical Request is in the following format.
     *
     * HTTPRequestMethod + '\n' + CanonicalURI + '\n' + CanonicalQueryString +
     * '\n' + CanonicalHeaders + '\n' + SignedHeaders + '\n' +
     * HexEncode(Hash(RequestPayload))
     */
    private String createCanonicalRequest(ClientRequestToken clientRequestToken) {
        String httpMethod, canonicalURI, canonicalQuery, canonicalHeader,
                hashedPayload, canonicalRequest;

        httpMethod = clientRequestToken.getHttpMethod();
        canonicalURI = clientRequestToken.getUri();
        canonicalQuery = getCanonicalQuery(clientRequestToken.getQuery());
        canonicalHeader = createCanonicalHeader(clientRequestToken);
        hashedPayload = createHashedPayload(clientRequestToken);

        canonicalRequest = String.format("%s\n%s\n%s\n%s\n%s\n%s",
                httpMethod,
                canonicalURI,
                canonicalQuery,
                canonicalHeader,
                clientRequestToken.getSignedHeaders(),
                hashedPayload);

        return canonicalRequest;
    }

    /**
     * Create a canonical request for seed request of a chunked request.
     *
     * HTTPRequestMethod + '\n' + CanonicalURI + '\n' + CanonicalQueryString +
     * '\n' + CanonicalHeaders + '\n' + SignedHeaders + '\n' +
     * STREAMING-AWS4-HMAC-SHA256-PAYLOAD
     *
     * @param clientRequestToken
     * @return
     */
    private String createCanonicalRequestChunkedSeed(
            ClientRequestToken clientRequestToken) {
        String httpMethod, canonicalURI, canonicalQuery, canonicalHeader,
                canonicalRequest;

        httpMethod = clientRequestToken.getHttpMethod();
        canonicalURI = clientRequestToken.getUri();
        canonicalQuery = getCanonicalQuery(clientRequestToken.getQuery());
        canonicalHeader = createCanonicalHeader(clientRequestToken);

        canonicalRequest = String.format("%s\n%s\n%s\n%s\n%s\n%s",
                httpMethod,
                canonicalURI,
                canonicalQuery,
                canonicalHeader,
                clientRequestToken.getSignedHeaders(),
                STREAMING_AWS4_HMAC_SHA256_PAYLOAD);

        return canonicalRequest;
    }

    /**
     * Calculate the hash of Payload using the following formula. HashedPayload
     * = Lowercase(HexEncode(Hash(requestPayload)))
     *
     * @param clientRequestToken
     * @return Hashed Payload
     */
    private String createHashedPayload(ClientRequestToken clientRequestToken) {
        if (clientRequestToken.getRequestHeaders().get("x-amz-content-sha256") != null) {
            return clientRequestToken.getRequestHeaders().get("x-amz-content-sha256");
        } else {
            String hashMethodName = AWSSign.AWSHashFunction.get(
                    clientRequestToken.getSigningAlgorithm());
            Method method;
            String hashedPayload = "";
            try {
                method = BinaryUtil.class.getMethod(hashMethodName, byte[].class);
                byte[] hashedText = (byte[]) method.invoke(null,
                        clientRequestToken.getRequestPayload().getBytes());
                hashedPayload = new String(BinaryUtil.encodeToHex(hashedText)).toLowerCase();
            } catch (NoSuchMethodException | SecurityException |
                    IllegalAccessException | IllegalArgumentException |
                    InvocationTargetException ex) {
            }

            return hashedPayload;
        }
    }

    /**
     * The string to sign includes meta information about your request and about
     * the canonical request.
     *
     * Structure of String to sign- Algorithm + '\n' + RequestDate + '\n' +
     * CredentialScope + '\n' + HashedCanonicalRequest
     */
    private String createStringToSign(String canonicalRequest,
            ClientRequestToken clientRequestToken) {
        String stringToSign, requestDate, hexEncodedCRHash;

        requestDate = clientRequestToken.getRequestHeaders().get("x-amz-date");
        hexEncodedCRHash = BinaryUtil.hexEncodedHash(canonicalRequest);

        stringToSign = String.format("%s\n%s\n%s\n%s",
                clientRequestToken.getSigningAlgorithm(), requestDate,
                clientRequestToken.getCredentialScope(), hexEncodedCRHash);

        return stringToSign;
    }

    /**
     * The string to sign includes meta information about your request and about
     * the canonical request.
     *
     * Structure of String to sign Algorithm + '\n' + RequestDate + '\n' +
     * CredentialScope + '\n' + HashedCanonicalRequest
     */
    private String createStringToSignChunked(
            ClientRequestToken clientRequestToken) {
        String stringToSign, requestDate, prevSign, hashCurrentChunk;
        String hashEmptyInput
                = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

        requestDate = clientRequestToken.getRequestHeaders().get("x-amz-date");
        prevSign = clientRequestToken.getRequestHeaders()
                .get("previous-signature-sha256");

        hashCurrentChunk = clientRequestToken.getRequestHeaders()
                .get("x-amz-content-sha256");

        stringToSign = String.format("AWS4-HMAC-SHA256-PAYLOAD\n%s\n%s\n%s\n%s\n%s",
                requestDate, clientRequestToken.getCredentialScope(), prevSign,
                hashEmptyInput, hashCurrentChunk
        );

        return stringToSign;
    }

    /**
     * Before calculating the signature, derive a signing key secret access key.
     *
     * Algorithm-
     *
     * kSecret = Your AWS Secret Access Key kDate = HMAC("AWS4" + kSecret, Date)
     * kRegion = HMAC(kDate, Region) kService = HMAC(kRegion, Service) kSigning
     * = HMAC(kService, "aws4_request")
     */
    private byte[] deriveSigningKey(ClientRequestToken clientRequestToken,
            String secretKey) {
        try {
            byte[] kSecret = ("AWS4" + secretKey).getBytes("UTF-8");
            byte[] kDate = BinaryUtil.hmacSHA256(kSecret,
                    clientRequestToken.getDate().getBytes("UTF-8"));

            byte[] kRegion = BinaryUtil.hmacSHA256(kDate,
                    clientRequestToken.getRegion().getBytes("UTF-8"));

            byte[] kService = BinaryUtil.hmacSHA256(kRegion,
                    clientRequestToken.getService().getBytes("UTF-8"));

            byte[] kSigning = BinaryUtil.hmacSHA256(kService,
                    "aws4_request".getBytes("UTF-8"));
            return kSigning;
        } catch (UnsupportedEncodingException ex) {
        }

        return null;
    }

    /**
     * Return the signature of the request.
     *
     * signature = HexEncode(HMAC(derived-signing-key, string-to-sign))
     */
    private String calculateSignature(byte[] derivedSigningKey, String stringToSign) {
        try {
            byte[] signature = BinaryUtil.hmacSHA256(derivedSigningKey,
                    stringToSign.getBytes("UTF-8"));
            return BinaryUtil.toHex(signature);
        } catch (UnsupportedEncodingException ex) {
        }

        return null;
    }

    /**
     * The canonical headers consist of a list of all the HTTP headers are
     * include as a part of the AWS request. Convert all header names to
     * lowercase and trim excess white space characters out of the header
     * values.
     *
     * Canonical Header = CanonicalHeadersEntry0 + CanonicalHeadersEntry1 + ...
     * + CanonicalHeadersEntryN
     *
     * CanonicalHeadersEntry = Lowercase(HeaderName) + ':' +
     * Trimall(HeaderValue) + '\n'
     */
    private String createCanonicalHeader(ClientRequestToken clientRequestToken) {
        String headerValue;
        String canonicalHeader = "";
        Map<String, String> requestHeaders = clientRequestToken.getRequestHeaders();

        for (String s : clientRequestToken.getSignedHeaders().split(";")) {
            if (s.equalsIgnoreCase("content-type")) {
                headerValue = requestHeaders.get(s).trim();

                /*
                 * Strangely, the aws .net sdk doesn't send the content type.
                 * Hence the content type is hard coded.
                 */
                if (headerValue.isEmpty()) {
                    canonicalHeader += "content-type:application/x-www-form-urlencoded;"
                            + " charset=utf-8\n";
                } else {
                    canonicalHeader += String.format("%s:%s\n", s, headerValue);
                }
            } else {
                headerValue = requestHeaders.get(s).trim();
                canonicalHeader += String.format("%s:%s\n", s, headerValue);
            }
        }

        return canonicalHeader;
    }

    /**
     * To construct the canonical query string, complete the following steps:
     *
     * 1. URI-encode each parameter name and value according to the following
     * rules: a. Do not URL-encode any of the unreserved characters that RFC
     * 3986 defines: A-Z, a-z, 0-9, hyphen ( - ), underscore ( _ ), period ( .
     * ), and tilde ( ~ ). b. Percent-encode all other characters with %XY,
     * where X and Y are hexadecimal characters (0-9 and uppercase A-F). For
     * example, the space character must be encoded as %20 (not using '+', as
     * some encoding schemes do) and extended UTF-8 characters must be in the
     * form %XY%ZA%BC.
     *
     * 2. Sort the encoded parameter names by character code (that is, in strict
     * ASCII order). For example, a parameter name that begins with the
     * uppercase letter F (ASCII code 70) precedes a parameter name that begins
     * with a lowercase letter b (ASCII code 98).
     *
     * 3. Build the canonical query string by starting with the first parameter
     * name in the sorted list.
     *
     * 4. For each parameter, append the URI-encoded parameter name, followed by
     * the character '=' (ASCII code 61), followed by the URI-encoded parameter
     * value. Use an empty string for parameters that have no value.
     *
     * 5. Append the character '&' (ASCII code 38) after each parameter value
     * except for the last value in the list.
     */
    private String getCanonicalQuery(String query) {
        if (query == null || query.isEmpty()) {
            return "";
        }

        Map<String, String> queryParams = new TreeMap<>();
        String[] tokens = query.split("&");
        for (String token : tokens) {
            String[] subTokens = token.split("=");
            if (subTokens.length == 2) {
                queryParams.put(subTokens[0], subTokens[1]);
            } else {
                queryParams.put(subTokens[0], "");
            }
        }

        String canonicalString = "";
        Iterator<Map.Entry<String, String>> entries = queryParams.entrySet().iterator();
        while (entries.hasNext()) {
            Map.Entry<String, String> entry = entries.next();
            try {
                canonicalString += URLEncoder.encode(entry.getKey(), "UTF-8") + "=";
                canonicalString += URLEncoder.encode(entry.getValue(), "UTF-8");
            } catch (UnsupportedEncodingException ex) {

            }

            if (entries.hasNext()) {
                canonicalString += "&";
            }
        }

        return canonicalString;
    }
}

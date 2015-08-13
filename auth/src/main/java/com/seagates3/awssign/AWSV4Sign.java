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

package com.seagates3.awssign;

import java.io.UnsupportedEncodingException;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.seagates3.authserver.ClientRequest;
import com.seagates3.model.Requestor;
import com.seagates3.util.BinaryUtil;

public class AWSV4Sign {
    Requestor requestor;
    ClientRequest request;

    public AWSV4Sign(ClientRequest request, Requestor requestor) {
        this.request = request;
        this.requestor = requestor;
    }

    /*
     * Authenticate the user.
     * If the user is authenticated
     *   return requestor object
     * else
     *   return null.
     */
    public Boolean authenticate() {
        String canonicalRequest, stringToSign, signature;
        byte[] signingKey;

        canonicalRequest = createCanonicalRequest();

        stringToSign = createStringToSign(canonicalRequest);

        signingKey = deriveSigningKey();

        signature = calculateSignature(signingKey ,stringToSign);

        return signature.equals(request.getSignature());
    }

    /*
     * Canonical Request is in the following format.
     *
     * HTTPRequestMethod + '\n' +
     * CanonicalURI + '\n' +
     * CanonicalQueryString + '\n' +
     * CanonicalHeaders + '\n' +
     * SignedHeaders + '\n' +
     * HexEncode(Hash(RequestPayload))
     */
    private String createCanonicalRequest() {
        String httpMethod, canonicalURI, canonicalQuery, canonicalHeader,
                hashedPayload, canonicalRequest;
        httpMethod = request.getMethod();
        canonicalURI = request.getCanonicalUri();
        canonicalQuery = request.getCanonicalQuery();
        canonicalHeader = request.getCanonicalHeader();
        hashedPayload = request.getHashedPayLoad();

        canonicalRequest = String.format("%s\n%s\n%s\n%s\n%s\n%s",
                httpMethod,
                canonicalURI,
                canonicalQuery,
                canonicalHeader,
                request.getSignedHeaders(),
                hashedPayload);

        return canonicalRequest;
    }

    /*
     * The string to sign includes meta information about your request and
     * about the canonical request.
     *
     * Structure of String to sign
     * Algorithm + '\n' +
     * RequestDate + '\n' +
     * CredentialScope + '\n' +
     * HashedCanonicalRequest
     */
    private String createStringToSign(String canonicalRequest) {
        String stringToSign, requestDate, hexEncodedCRHash;

        requestDate = request.getRequestDate();
        hexEncodedCRHash = BinaryUtil.hexEncodedHash(canonicalRequest);

        stringToSign = String.format("%s\n%s\n%s\n%s", request.getAlgorithm(),
                requestDate, request.getCredentialScope(), hexEncodedCRHash);

        return stringToSign;
    }

    /*
     * Before calculating the signature, derive a signing key secret access key.
     *
     * Algorithm-
     *
     * kSecret = Your AWS Secret Access Key
     * kDate = HMAC("AWS4" + kSecret, Date)
     * kRegion = HMAC(kDate, Region)
     * kService = HMAC(kRegion, Service)
     * kSigning = HMAC(kService, "aws4_request")
     */
    private byte[] deriveSigningKey() {
        try {
            byte[] kSecret = ("AWS4" + requestor.getAccesskey().getSecretKey())
                    .getBytes("UTF-8");
            byte[] kDate = BinaryUtil.hmac(kSecret,
                    request.getDate().getBytes("UTF-8"));

            byte[] kRegion = BinaryUtil.hmac(kDate,
                    request.getRegion().getBytes("UTF-8"));

            byte[] kService = BinaryUtil.hmac(kRegion,
                    request.getService().getBytes("UTF-8"));

            byte[] kSigning = BinaryUtil.hmac(kService,
                    "aws4_request".getBytes("UTF-8"));
            return kSigning;
        } catch (UnsupportedEncodingException ex) {
            Logger.getLogger(AWSV4Sign.class.getName()).log(Level.SEVERE, null, ex);
        }

        return null;
    }

    /*
     * Return the signature of the request.
     *
     * signature = HexEncode(HMAC(derived-signing-key, string-to-sign))
     */
    private String calculateSignature(byte[] derivedSigningKey, String stringToSign) {
        try {
            byte[] signature = BinaryUtil.hmac(derivedSigningKey,
                    stringToSign.getBytes("UTF-8"));
            return BinaryUtil.toHex(signature);
        } catch (UnsupportedEncodingException ex) {
            Logger.getLogger(AWSV4Sign.class.getName()).log(Level.SEVERE, null, ex);
        }

        return null;
    }
}

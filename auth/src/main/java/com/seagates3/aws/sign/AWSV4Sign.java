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

package com.seagates3.aws.sign;

import java.io.UnsupportedEncodingException;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.seagates3.model.ClientRequestToken;
import com.seagates3.model.Requestor;
import com.seagates3.util.BinaryUtil;

public class AWSV4Sign implements AWSSign{

    /*
     * Return true if the signature is valid.
     */
    @Override
    public Boolean authenticate(ClientRequestToken clientRequestToken,
            Requestor requestor) {
        String canonicalRequest, stringToSign, signature;
        byte[] signingKey;

        canonicalRequest = createCanonicalRequest(clientRequestToken);

        stringToSign = createStringToSign(canonicalRequest, clientRequestToken);

        String secretKey = requestor.getAccesskey().getSecretKey();
        signingKey = deriveSigningKey(clientRequestToken, secretKey);

        signature = calculateSignature(signingKey ,stringToSign);

        return signature.equals(clientRequestToken.getSignature());
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
    private String createCanonicalRequest(ClientRequestToken clientRequestToken) {
        String httpMethod, canonicalURI, canonicalQuery, canonicalHeader,
                hashedPayload, canonicalRequest;

        httpMethod = clientRequestToken.getHttpMethod();
        canonicalURI = clientRequestToken.getCanonicalUri();
        canonicalQuery = clientRequestToken.getCanonicalQuery();
        canonicalHeader = clientRequestToken.getCanonicalHeader();
        hashedPayload = clientRequestToken.getHashedPayLoad();

        canonicalRequest = String.format("%s\n%s\n%s\n%s\n%s\n%s",
                httpMethod,
                canonicalURI,
                canonicalQuery,
                canonicalHeader,
                clientRequestToken.getSignedHeaders(),
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
    private String createStringToSign(String canonicalRequest,
            ClientRequestToken clientRequestToken) {
        String stringToSign, requestDate, hexEncodedCRHash;

        requestDate = clientRequestToken.getRequestDate();
        hexEncodedCRHash = BinaryUtil.hexEncodedHash(canonicalRequest);

        stringToSign = String.format("%s\n%s\n%s\n%s", clientRequestToken.getSigningAlgorithm(),
                requestDate, clientRequestToken.getCredentialScope(), hexEncodedCRHash);

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
    private byte[] deriveSigningKey(ClientRequestToken clientRequestToken, String secretKey) {
        try {
            byte[] kSecret = ("AWS4" + secretKey)
                    .getBytes("UTF-8");
            byte[] kDate = BinaryUtil.hmac(kSecret,
                    clientRequestToken.getDate().getBytes("UTF-8"));

            byte[] kRegion = BinaryUtil.hmac(kDate,
                    clientRequestToken.getRegion().getBytes("UTF-8"));

            byte[] kService = BinaryUtil.hmac(kRegion,
                    clientRequestToken.getService().getBytes("UTF-8"));

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

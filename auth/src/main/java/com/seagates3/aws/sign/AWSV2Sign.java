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

package com.seagates3.aws.sign;

import com.seagates3.model.ClientRequestToken;
import com.seagates3.model.Requestor;

public class AWSV2Sign implements AWSSign {

    /*
     * Return true if the signature is valid.
     */
    @Override
    public Boolean authenticate(ClientRequestToken clientRequestToken, Requestor requestor) {
        String canonicalizedResource, stringToSign, signature;
        byte[] signingKey;

        canonicalizedResource = createCanonicalizedResource(clientRequestToken);
        stringToSign = createStringToSign(clientRequestToken);

//        stringToSign = createStringToSign(canonicalRequest, clientRequestToken);
//
//        String secretKey = requestor.getAccesskey().getSecretKey();
//        signingKey = deriveSigningKey(clientRequestToken, secretKey);

        signature = "";//calculateSignature(signingKey ,stringToSign);

        return signature.equals(clientRequestToken.getSignature());
    }

    private String createCanonicalizedResource(ClientRequestToken clientRequestToken) {
        return "";
    }

    /*
     * String to sign is in the following format.
     *
     * HTTPRequestMethod + '\n' +
     * Content-MD5 + "\n" +
     * Content-Type + "\n" +
     * Date + "\n" +
     * CanonicalizedAmzHeaders +
     * CanonicalizedResource;
     */
    private String createStringToSign(ClientRequestToken clientRequestToken) {
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

}

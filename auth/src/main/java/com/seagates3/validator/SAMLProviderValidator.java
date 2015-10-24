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
 * Original creation date: 14-Oct-2015
 */

package com.seagates3.validator;

import java.util.Map;


public class SAMLProviderValidator extends AbstractValidator {

    private final ValidatorHelper validatorHelper;

    public SAMLProviderValidator() {
        validatorHelper = new ValidatorHelper();
    }

    @Override
    public Boolean create(Map<String, String> requestBody) {
        if(!validatorHelper.validName(requestBody.get("Name"))) {
            return false;
        }

        /*
         * Saml metadata doc is supposed to be in base 64 format.
         * But looks like Netty is decoding from base 64 to ascii.
         * Investigate this issue.
         */
        // if(!BinaryUtil.isBase64Encoded(requestBody.get("SAMLMetadataDocument"))) {
        //     return false;
        // }

        return validatorHelper.validSAMLMetadata(
                requestBody.get("SAMLMetadataDocument"));
    }

    /*
     * Validate the input parameters for delete saml provider request.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        return requestBody.containsKey("SAMLProviderArn");
    }

    /*
     * Validate the input parameters for update user request.
     */
    @Override
    public Boolean update(Map<String, String> requestBody) {
        if(!requestBody.containsKey("SAMLMetadataDocument")) {
            return false;
        }

        return requestBody.containsKey("SAMLProviderArn");
    }
}

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

/**
 * Validate the input for SAML provider APIs - Create, Delete and update.
 */
public class SAMLProviderValidator extends AbstractValidator {
    /**
     * Validate the input parameters for create SAML Provider request.
     * SAML provider name is required.
     * SAML metadata is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean create(Map<String, String> requestBody) {
        if(!S3ValidatorUtil.isValidSamlProviderName(requestBody.get("Name"))) {
            return false;
        }

        return S3ValidatorUtil.isValidSAMLMetadata(requestBody.get("SAMLMetadataDocument"));
    }

    /**
     * Validate the input parameters for delete user request.
     * SAML Provider ARN is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        return S3ValidatorUtil.isValidARN(requestBody.get("SAMLProviderArn"));
    }

    /**
     * Validate the input parameters for list SAML providers request.
     * This API doesn't require an input.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true.
     */
    @Override
    public Boolean list(Map<String, String> requestBody) {
        return true;
    }

    /**
     * Validate the input parameters for update SAML provider request.
     * SAML provider ARN is required.
     * SAML metadata is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true.
     */
    @Override
    public Boolean update(Map<String, String> requestBody) {
        if(!S3ValidatorUtil.isValidARN(requestBody.get("SAMLProviderArn"))) {
            return false;
        }

        return S3ValidatorUtil.isValidSAMLMetadata(requestBody.get("SAMLMetadataDocument"));
    }
}

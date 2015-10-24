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

package com.seagates3.validator;

import java.util.Map;

public class AccessKeyValidator extends AbstractValidator {

    private final ValidatorHelper validatorHelper;

    public AccessKeyValidator() {
        validatorHelper = new ValidatorHelper();
    }

    /*
     * Validate the input parameters for create access key request.
     */
    @Override
    public Boolean create(Map<String, String> requestBody) {
        if(requestBody.containsKey("UserName")) {
            return validatorHelper.validName(requestBody.get("UserName"));
        }

        return true;
    }

    /*
     * Validate the input parameters for delete access key request.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        if(! requestBody.containsKey("AccessKeyId")) {
            return false;
        }

        if(!validatorHelper.validAccessKeyId(requestBody.get("AccessKeyId"))) {
            return false;
        }

        // User name should be at least 1 characted long and at max 128 char long.
        if(requestBody.containsKey("UserName")) {
            return validatorHelper.validName(requestBody.get("UserName"));
        }

        return true;
    }

    /*
     * Validate the input parameters for list access key request.
     *
     * To do
     * Validate Marker and MaxItems
     */
    @Override
    public Boolean list(Map<String, String> requestBody) {
        if(requestBody.containsKey("UserName")) {
            return validatorHelper.validName(requestBody.get("UserName"));
        }

        return true;
    }

    /*
     * Validate the input parameters for update access key request.
     */
    @Override
    public Boolean update(Map<String, String> requestBody) {
        if(! requestBody.containsKey("AccessKeyId")) {
            return false;
        }

        if(!requestBody.containsKey("Status")) {
            return false;
        }

        // Accepted values for status are 'Active' or 'Inactive'
        if(!validatorHelper.validStatus(requestBody.get("Status"))) {
            return false;
        }

        // User name should be at least 1 characted long and at max 128 char long.
        if(requestBody.containsKey("UserName")) {
            return validatorHelper.validName(requestBody.get("UserName"));
        }

        return true;
    }
}

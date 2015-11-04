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
 * Original creation date: 31-Oct-2015
 */

package com.seagates3.validator;

import java.util.Map;

public class RoleValidator extends AbstractValidator {
    /*
     * Validate the input parameters for create role request.
     */
    @Override
    public Boolean create(Map<String, String> requestBody) {
        if(!requestBody.containsKey("AssumeRolePolicyDocument")) {
            return false;
        }

        if(!requestBody.containsKey("RoleName")) {
            return false;
        }

        if(!validPolicy(requestBody.get("AssumeRolePolicyDocument"))) {
            return false;
        }

        if(!validRoleName(requestBody.get("RoleName"))) {
            return false;
        }

        if(requestBody.containsKey("Path")) {
            return validatorHelper.validPath(requestBody.get("Path"));
        }

        return true;
    }

    /*
     * Validate the input parameters for delete role request.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        if(!requestBody.containsKey("RoleName")) {
            return false;
        }

        return validRoleName(requestBody.get("RoleName"));
    }

    /*
     * Validate the input parameters for list role request.
     */
    @Override
    public Boolean list(Map<String, String> requestBody) {
        if(requestBody.containsKey("PathPrefix")) {
            if(!validatorHelper.validPathPrefix(requestBody.get("PathPrefix"))) {
                return false;
            }
        }

        if(requestBody.containsKey("MaxItems")) {
            int maxItems = Integer.parseInt(requestBody.get("MaxItems"));
            if(!validatorHelper.validMaxItems(maxItems)) {
                return false;
            }
        }

        if(requestBody.containsKey("Marker")) {
            int marker = Integer.parseInt(requestBody.get("Marker"));
            return validatorHelper.validMarker(marker);
        }

        return true;
    }

    /*
     * policy should be atleast 1 character long and at max 2048 char long.
     */
    private Boolean validPolicy(String policy) {
        return !(policy.length() < 1 || policy.length() > 2048);
    }

    /*
     * Role name should be atleast 1 character long and at max 2048 char long.
     */
    private Boolean validRoleName(String name) {
        return !(name.length() < 1 || name.length() > 64);
    }
}

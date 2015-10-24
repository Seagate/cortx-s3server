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

public class UserValidator extends AbstractValidator {

    private final ValidatorHelper validatorUtil;

    public UserValidator() {
        validatorUtil = new ValidatorHelper();
    }
    /*
     * Validate the input parameters for create user request.
     */
    @Override
    public Boolean create(Map<String, String> requestBody) {
        if(!requestBody.containsKey("UserName")) {
            return false;
        }

        if(!validatorUtil.validName(requestBody.get("UserName"))) {
            return false;
        }

        if(requestBody.containsKey("Path")) {
            return validatorUtil.validPath(requestBody.get("Path"));
        }

        return true;
    }

    /*
     * Validate the input parameters for delete user request.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        if(!requestBody.containsKey("UserName")) {
            return false;
        }

        return validatorUtil.validName(requestBody.get("UserName"));
    }

    /*
     * Validate the input parameters for list users request.
     *
     * To do
     * Validate Marker and MaxItems
     */
    @Override
    public Boolean list(Map<String, String> requestBody) {
        if(requestBody.containsKey("PathPrefix")) {
            return validatorUtil.validPath(requestBody.get("PathPrefix"));
        }

        return true;
    }

    /*
     * Validate the input parameters for update user request.
     */
    @Override
    public Boolean update(Map<String, String> requestBody) {
        if(!requestBody.containsKey("UserName")) {
            return false;
        }

        if(!validatorUtil.validName(requestBody.get("UserName"))) {
            return false;
        }

        if(requestBody.containsKey("NewUserName")) {
            if(!validatorUtil.validName(requestBody.get("UserName"))) {
                return false;
            }
        }

        if(requestBody.containsKey("NewPath")) {
            if(!validatorUtil.validPath(requestBody.get("NewPath"))) {
                return false;
            }
        }
        return true;
    }
}

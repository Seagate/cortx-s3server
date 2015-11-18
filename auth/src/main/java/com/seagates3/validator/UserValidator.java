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
 * Original creation date: 17-Sep-2015
 */

package com.seagates3.validator;

import java.util.Map;

/**
 * Validate the input for User APIs - Create, Delete, List and update.
 */
public class UserValidator extends AbstractValidator {

    /**
     * Validate the input parameters for create user request.
     * User name is required.
     * Path is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean create(Map<String, String> requestBody) {
        if (requestBody.containsKey("Path")) {
            return S3ValidatorUtil.isValidPath(requestBody.get("Path"));
        }

        return S3ValidatorUtil.isValidName(requestBody.get("UserName"));
    }

    /**
     * Validate the input parameters for delete user request.
     * User name is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        return S3ValidatorUtil.isValidName(requestBody.get("UserName"));
    }

    /**
     * Validate the input parameters for update user request.
     * User name is required.
     * New User name is optional.
     * New Path is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean update(Map<String, String> requestBody) {
        if (requestBody.containsKey("NewUserName")) {
            if (!S3ValidatorUtil.isValidName(requestBody.get("NewUserName"))) {
                return false;
            }
        }

        if (requestBody.containsKey("NewPath")) {
            if (!S3ValidatorUtil.isValidPath(requestBody.get("NewPath"))) {
                return false;
            }
        }

        return S3ValidatorUtil.isValidName(requestBody.get("UserName"));
    }
}

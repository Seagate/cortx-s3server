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
 * Validate the input for Access Key APIs - Create, Delete, List and update.
 */
public class AccessKeyValidator extends AbstractValidator {

    /**
     * Validate the input parameters for create access key request.
     * User name is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean create(Map<String, String> requestBody) {
        if (requestBody.containsKey("UserName")) {
            return S3ValidatorUtil.isValidName(requestBody.get("UserName"));
        }

        return true;
    }

    /**
     * Validate the input parameters for delete access key request.
     * Access key id is required.
     * User name is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean delete(Map<String, String> requestBody) {
        if (requestBody.containsKey("UserName")) {
            return S3ValidatorUtil.isValidName(requestBody.get("UserName"));
        }

        return S3ValidatorUtil.isValidAccessKeyId(requestBody.get("AccessKeyId"));
    }

    /**
     * Validate the input parameters for list access keys request.
     * Path prefix is optional.
     * Max Items is optional.
     * Marker is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean list(Map<String, String> requestBody) {
        if (requestBody.containsKey("UserName")) {
            if (!S3ValidatorUtil.isValidName(requestBody.get("UserName"))) {
                return false;
            }
        }

        if (requestBody.containsKey("MaxItems")) {
            if (!S3ValidatorUtil.isValidMaxItems(requestBody.get("MaxItems"))) {
                return false;
            }
        }

        if (requestBody.containsKey("Marker")) {
            return S3ValidatorUtil.isValidMarker(requestBody.get("Marker"));
        }

        return true;
    }

    /**
     * Validate the input parameters for update access key request.
     * Access key id is required.
     * Status is required.
     * User Name is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean update(Map<String, String> requestBody) {
        if (!S3ValidatorUtil.isValidAccessKeyStatus(requestBody.get("Status"))) {
            return false;
        }

        if (requestBody.containsKey("UserName")) {
            if (!S3ValidatorUtil.isValidName(requestBody.get("UserName"))) {
                return false;
            }
        }

        return S3ValidatorUtil.isValidAccessKeyId(requestBody.get("AccessKeyId"));
    }
}

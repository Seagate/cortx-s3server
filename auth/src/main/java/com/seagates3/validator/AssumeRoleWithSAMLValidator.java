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
 * Original creation date: 28-Oct-2015
 */

package com.seagates3.validator;

import com.seagates3.util.BinaryUtil;
import java.util.Map;

public class AssumeRoleWithSAMLValidator extends AbstractValidator{
    @Override
    public Boolean create(Map<String, String> requestBody) {
        Boolean isValid;

        if(!requestBody.containsKey("PrincipalArn") ||
                !requestBody.containsKey("RoleArn") ||
                !requestBody.containsKey("SAMLAssertion")) {
            return false;
        }

        isValid = validatePrincipalArn(requestBody.get("PrincipalArn"));
        if(!isValid) {
            return false;
        }

        isValid = validateRoleArn(requestBody.get("RoleArn"));
        if(!isValid) {
            return false;
        }

        isValid = validateSAMLAssertion(requestBody.get("SAMLAssertion"));
        if(!isValid) {
            return false;
        }

        if(requestBody.containsKey("DurationSeconds")) {
            isValid = validateTokenDuration(requestBody.get("DurationSeconds"));
            if(!isValid) {
                return false;
            }
        }

        if(requestBody.containsKey("Policy")) {
            return validatePolicy(requestBody.get("Policy"));
        }

        return isValid;
    }

    private Boolean validateTokenDuration(String duration) {
        int durationInSeconds = Integer.parseInt(duration);

        return !(durationInSeconds > 3600 || durationInSeconds < 900);
    }

    private Boolean validatePolicy(String policy) {
        return !(policy.length() < 1 || policy.length() > 2048);
    }

    /*
     * TODO
     * ARN should be minimum 20 characters long
     */
    private Boolean validatePrincipalArn(String arn) {
        return !(arn.length() < 1 || arn.length() > 2048);
    }

    /*
     * TODO
     * ARN should be minimum 20 characters long
     */
    private Boolean validateRoleArn(String roleArn) {
        return !(roleArn.length() < 1 || roleArn.length() > 2048);
    }

    private Boolean validateSAMLAssertion(String assertion) {
        Boolean base64Encoded = BinaryUtil.isBase64Encoded(assertion);
        if(!base64Encoded) {
            return false;
        }

        return !(assertion.length() < 4 || assertion.length() > 50000);
    }
}

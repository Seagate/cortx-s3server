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

public class FederationTokenValidator extends AbstractValidator {

    public Boolean create(Map<String, String> requestBody) {
        Boolean isValid;
        if(!requestBody.containsKey("Name")) {
            return false;
        }

        isValid = validUserName(requestBody.get("Name"));
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

        return true;
    }

    private Boolean validUserName(String userName) {
        return !(userName.length() < 2 || userName.length() > 32);
    }

    private Boolean validateTokenDuration(String duration) {
        int durationInSeconds = Integer.parseInt(duration);

        return !(durationInSeconds > 129600 || durationInSeconds < 900);
    }

    private Boolean validatePolicy(String policy) {
        return !(policy.length() < 1 || policy.length() > 2048);
    }
}

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

package com.seagates3.awssign;

import java.util.Date;

import com.seagates3.model.AccessKey;
import com.seagates3.authserver.ClientRequest;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.xml.XMLResponseGenerator;
import com.seagates3.util.DateUtil;

public class RequestorAuthenticator {
    public static ServerResponse authenticate(Requestor requestor, ClientRequest request) {
        XMLResponseGenerator responseGenerator = new XMLResponseGenerator();
        /*
        * Authenticate ther user before processing the request.
        */
        if(!requestor.exists()) {
            return responseGenerator.noSuchEntity();
        }

        AccessKey accessKey = requestor.getAccesskey();
        if(!accessKey.isAccessKeyActive()) {
            return responseGenerator.inactiveAccessKey();
        }

        if(requestor.isFederatedUser()) {
            if(!accessKey.getToken().equals(request.getSessionToken())) {
                return responseGenerator.invalidClientTokenId();
            }

            Date currentDate = new Date();
            Date expiryDate = DateUtil.toDate(accessKey.getExpiry());

            if(currentDate.after(expiryDate)) {
                return responseGenerator.expiredCredential();
            }
        }

        AWSV4Sign authenticator =
                new AWSV4Sign(request, requestor);

        Boolean isRequestorAuthenticated = authenticator.authenticate();
        if(!isRequestorAuthenticated) {
            return responseGenerator.incorrectSignature();
        }

        return responseGenerator.ok();
    }
}

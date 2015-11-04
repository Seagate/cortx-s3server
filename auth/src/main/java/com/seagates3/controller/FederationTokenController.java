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

package com.seagates3.controller;

import java.util.Date;
import java.util.Map;

import com.seagates3.model.Requestor;
import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKey.AccessKeyStatus;
import com.seagates3.model.User;
import com.seagates3.response.generator.xml.FederationTokenResponseGenerator;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.DateUtil;
import com.seagates3.util.KeyGenUtil;

public class FederationTokenController extends AbstractController{
    FederationTokenResponseGenerator fedTokenResponse;

    public FederationTokenController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        fedTokenResponse = new FederationTokenResponseGenerator();
    }

    /*
     * TODO
     * Store user policy
     */
    @Override
    public ServerResponse create() throws DataAccessException {
        String userName = requestBody.get("Name");

        int duration;
        if(requestBody.containsKey("DurationSeconds")) {
            duration = Integer.parseInt(requestBody.get("DurationSeconds"));
        } else {
            duration = 43200;
        }

        User user = createUser(requestor.getAccountName(), userName);

        AccessKey accessKey = createAccessKey(user, duration);
        if(accessKey == null) {
            return fedTokenResponse.internalServerError();
        }

        return fedTokenResponse.create(user, accessKey);
    }

    /*
     * Search for the user in the data base. If the user exists, return the user.
     * Else, create a new user.
     */
    private User createUser(String accountName, String userName) throws DataAccessException {
        UserDAO userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        User user = userDAO.find(accountName, userName);

        if(! user.exists()) {
            user.setId(KeyGenUtil.userId());
            user.setUserType(User.UserType.IAM_FED_USER);
            userDAO.save(user);

        }
        return user;
    }

    /*
     * Create temporary access key, secret key and session token for the user.
     */
    private AccessKey createAccessKey(User user, long timeToExpire) throws DataAccessException {
        AccessKeyDAO accessKeyDAO =
                (AccessKeyDAO) DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);
        AccessKey accessKey = new AccessKey();

        String strToEncode = user.getId() + System.currentTimeMillis();

        accessKey.setUserId(user.getId());
        accessKey.setAccessKeyId(KeyGenUtil.userAccessKeyId());
        accessKey.setSecretKey(KeyGenUtil.userSercretKey(strToEncode));

        strToEncode = user.getId() + System.currentTimeMillis();
        accessKey.setToken(KeyGenUtil.userSercretKey(strToEncode));
        accessKey.setStatus(AccessKeyStatus.ACTIVE);

        long currentTime = DateUtil.getCurrentTime();
        Date expiryDate = new Date(currentTime + (timeToExpire * 1000));
        accessKey.setExpiry(DateUtil.toServerResponseFormat(expiryDate));

        accessKeyDAO.save(accessKey);

        return accessKey;
    }
}

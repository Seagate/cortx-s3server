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

package com.seagates3.controller;

import java.util.Date;

import java.util.Map;
import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.SAMLProviderDAO;
import com.seagates3.dao.UserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Requestor;
import com.seagates3.model.SAMLProvider;
import com.seagates3.model.SAMLResponse;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.xml.AssumeRoleWithSAMLResponseGenerator;
import com.seagates3.util.BinaryUtil;
import com.seagates3.util.DateUtil;
import com.seagates3.util.KeyGenUtil;
import com.seagates3.util.SAMLUtil;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;



public class AssumeRoleWithSAMLController extends AbstractController {
    SAMLProviderDAO samlProviderDao;
    AssumeRoleWithSAMLResponseGenerator responseGenerator;

    /*
     * Constructor
     */
    public AssumeRoleWithSAMLController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        responseGenerator = new AssumeRoleWithSAMLResponseGenerator();
        samlProviderDao = (SAMLProviderDAO) DAODispatcher.getResourceDAO(DAOResource.SAML_PROVIDER);
    }

    @Override
    public ServerResponse create() throws DataAccessException {
        String samlAssertion = BinaryUtil.base64DecodeString(requestBody.get("SAMLAssertion"));
        SAMLResponse samlResponse = SAMLUtil.getSamlResponse(samlAssertion);

        if(!samlResponse.isSuccess()) {
            return responseGenerator.idpRejectedClaim();
        }

        if(!samlResponse.isValidSignature()) {
            return responseGenerator.invalidIdentityToken();
        }

        DateTime currentDateTime = DateTime.now(DateTimeZone.UTC);
        if(currentDateTime.isAfter(samlResponse.getIssueInstant().plusMinutes(5))) {
            return responseGenerator.expiredToken();
        }

        if(samlResponse.getNotBefore() != null) {
            if(currentDateTime.isBefore(samlResponse.getNotBefore())) {
                return responseGenerator.idpRejectedClaim();
            }
        }

        /*
         * check if SAML Response has "SessionNotOnOrAfter".
         * If Response has "SessionNotOnOrAfter", calculate the duration from
         * current time.
         * Else, set duration = 3600 (default)
         */
        int duration = -1;
        if(samlResponse.getNotOnOrAfter() != null) {
            if(currentDateTime.isAfter(samlResponse.getNotOnOrAfter())) {
                return responseGenerator.idpRejectedClaim();
            }

            duration = (int) ((samlResponse.getNotOnOrAfter().getMillis() -
                    currentDateTime.getMillis())/1000);
        }

        if(duration == -1 || duration > 3600 ) {
            duration = 3600;
        }

        /*
         * TODO
         * Implement Principal ARN.
         *
         * Temporary fix -
         * PrincipalARN should be in this format -
         * arn:seagate:iam::<account name>:<idp provider name>
         */

        String[]tokens = requestBody.get("PrincipalArn").split("arn:seagate:iam::");
        tokens = tokens[1].split(":");
        String accountName = tokens[0];
        String providerName = tokens[1];

        SAMLProvider samlProvider = samlProviderDao.find(accountName, providerName);
        if(!samlProvider.exists()) {
            return responseGenerator.noSuchEntity();
        }

        Date idpExpiry = DateUtil.toDate(samlProvider.getExpiry());
        if(DateTime.now().isAfter(idpExpiry.getTime())) {
            responseGenerator.idpRejectedClaim();
        }

        if(!samlProvider.getIssuer().equals(samlResponse.getIssuer())) {
            responseGenerator.invalidIdentityToken();
        }

        if((requestBody.get("DurationSeconds") != null) &&
                (duration > Integer.parseInt(requestBody.get("DurationSeconds")))) {
            duration = Integer.parseInt(requestBody.get("DurationSeconds"));
        }

        /*
         * TODO
         * Implement Role ARN.
         *
         * Temporary fix -
         * PrincipalARN should be in this format -
         * arn:seagate:iam::roles:<role name>
         */

        tokens = requestBody.get("RoleArn").split("arn:seagate:iam::");
        tokens = tokens[1].split(":");
        String roleName = tokens[1];
        User user = createRoleUser(accountName, roleName, samlResponse.getRoleSessionName());

        AccessKey accessKey = createAccessKey(user, duration);

        return responseGenerator.create(user, samlResponse, accessKey);
    }

    /*
     * Search for the role user.
     * If the user doesn't exist, create a new user
     */
    private User createRoleUser(String accountName, String role, String userName)
            throws DataAccessException {
        String roleName = String.format("%s/%s", role, userName);

        UserDAO userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        User user = userDAO.find(accountName, roleName);
        if(! user.exists()) {
            user.setId(KeyGenUtil.userId());
            user.setRoleName(roleName);
            user.setUserType(User.UserType.ROLE_USER);
            userDAO.save(user);
        }

        return user;
    }

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
        accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);

        long currentTime = DateUtil.getCurrentTime();
        Date expiryDate = new Date(currentTime + (timeToExpire * 1000));
        accessKey.setExpiry(DateUtil.toServerResponseFormat(expiryDate));

        accessKeyDAO.save(accessKey);

        return accessKey;
    }

}

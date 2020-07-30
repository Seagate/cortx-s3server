package com.seagates3.service;

import java.util.Date;

import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;
import com.seagates3.util.KeyGenUtil;

public class AccessKeyService {

    public static AccessKey createFedAccessKey(User user, long timeToExpire)
            throws DataAccessException {
        AccessKeyDAO accessKeyDAO = (AccessKeyDAO) DAODispatcher.getResourceDAO(
                DAOResource.ACCESS_KEY);
        AccessKey accessKey = new AccessKey();
        accessKey.setUserId(user.getId());
        accessKey.setId(KeyGenUtil.createUserAccessKeyId());
        accessKey.setSecretKey(KeyGenUtil.generateSecretKey());
        accessKey.setToken(KeyGenUtil.generateSecretKey());
        accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);

        long currentTime = DateUtil.getCurrentTime();
        Date expiryDate = new Date(currentTime + (timeToExpire * 1000));
        accessKey.setExpiry(DateUtil.toServerResponseFormat(expiryDate));

        accessKeyDAO.save(accessKey);

        return accessKey;
    }
}


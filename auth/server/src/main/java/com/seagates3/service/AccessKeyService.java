/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

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
        accessKey.setId(KeyGenUtil.createUserAccessKeyId(false));
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


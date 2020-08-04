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

package com.seagates3.dao;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.authserver.SSLContextProvider;
import com.seagates3.dao.ldap.LdapConnectionManager;
import com.seagates3.exception.ServerInitialisationException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class DAODispatcher {

    private static final String DAO_PACKAGE = "com.seagates3.dao";
    private static DAOProvider provider;
    private static final Logger LOGGER
            = LogManager.getLogger(SSLContextProvider.class.getName());

    public static void init() throws ServerInitialisationException {
        String dataSource = AuthServerConfig.getDataSource();
        DAODispatcher.provider = DAOProvider.valueOf(dataSource.toUpperCase());

        if ("LDAP".equals(dataSource.toUpperCase())) {
            LdapConnectionManager.initLdap();
            LOGGER.info("Initialized LDAP");
        }
    }

    public static Object getResourceDAO(DAOResource daoResource) {
        Class<?> validator;
        Object obj;
        String resourceDAOName = getResourceDAOName(daoResource.toString());

        try {
            validator = Class.forName(resourceDAOName);
            obj = validator.newInstance();
        } catch (ClassNotFoundException | SecurityException ex) {
            obj = null;
        } catch (IllegalAccessException | IllegalArgumentException | InstantiationException ex) {
            obj = null;
        }

        return obj;
    }

    private static String getResourceDAOName(String resourceName) {
        String resourceDAOName = resourceName + "Impl";

        return String.format("%s.%s.%s", DAO_PACKAGE,
                provider.toString().toLowerCase(), resourceDAOName);
    }
}

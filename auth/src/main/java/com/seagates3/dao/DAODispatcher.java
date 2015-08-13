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

package com.seagates3.dao;

public class DAODispatcher {
    static final String DAO_PACKAGE = "com.seagates3.dao";
    static DAOProvider provider;

    public static void Init(String dataSource) {
        DAODispatcher.provider = DAOProvider.valueOf(dataSource.toUpperCase());
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
        String resourceDAOName = provider.toString()+ resourceName + "DAO";

        return String.format("%s.%s.%s", DAO_PACKAGE,
                provider.toString().toLowerCase(), resourceDAOName);
    }
}

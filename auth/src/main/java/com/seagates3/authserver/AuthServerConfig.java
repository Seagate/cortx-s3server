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
 * Original creation date: 23-Oct-2015
 */

package com.seagates3.authserver;

import java.util.Properties;

public class AuthServerConfig {

    private static String[] endpoints;
    private static String defaultEndpoint;

    /*
     * Initialize the AuthServerConfig with server properties like
     * default endpoint, s3 endpoints etc.
     */
    public static void init(Properties authServerConfig) {
        endpoints = authServerConfig.getProperty("s3Endpoints").split(",");
        defaultEndpoint = authServerConfig.getProperty("defaultEndpoint");
    }

    public static String[] getEndpoints() {
        return endpoints;
    }

    public static String getDefaultEndpoint() {
        return defaultEndpoint;
    }
}

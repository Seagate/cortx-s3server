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

package com.seagates3.dao.ldap;

import java.io.UnsupportedEncodingException;
import java.util.Properties;

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPException;
import com.novell.ldap.connectionpool.PoolManager;


public class LdapConnectionManager{

    static PoolManager ldapPool;
    static String ldapLoginDN, ldapLoginPW;

    public static void initLdap(Properties authServerConfig) {
        try {
            ldapPool  = new PoolManager(authServerConfig.getProperty("ldapHost"),
                    Integer.parseInt(authServerConfig.getProperty("ldapPort")),
                    Integer.parseInt(authServerConfig.getProperty("maxCons")),
                    Integer.parseInt(authServerConfig.getProperty("maxSharedCons")),
                    null);

            ldapLoginDN = authServerConfig.getProperty("ldapLoginDN");
            ldapLoginPW = authServerConfig.getProperty("ldapLoginPW");
        } catch (LDAPException ex) {
        }
    }

    public static LDAPConnection getConnection() {
        LDAPConnection lc = null;
        try {
            lc = ldapPool.getBoundConnection(
                    ldapLoginDN, ldapLoginPW.getBytes("UTF-8"));
        } catch (LDAPException | InterruptedException | UnsupportedEncodingException ex) {
        }

        return lc;
    }

    public static void releaseConnection(LDAPConnection lc) {
        ldapPool.makeConnectionAvailable(lc);
    }
}

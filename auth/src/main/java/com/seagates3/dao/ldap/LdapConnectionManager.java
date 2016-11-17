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
 * Original creation date: 17-Sep-2015
 */
package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPException;
import com.novell.ldap.connectionpool.PoolManager;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.ServerInitialisationException;
import com.seagates3.fi.FaultPoints;
import java.io.UnsupportedEncodingException;

public class LdapConnectionManager {

    static PoolManager ldapPool;
    static String ldapLoginDN, ldapLoginPW;

    public static void initLdap()
            throws ServerInitialisationException {
        try {
            ldapPool = new PoolManager(AuthServerConfig.getLdapHost(),
                    AuthServerConfig.getLdapPort(),
                    AuthServerConfig.getLdapMaxConnections(),
                    AuthServerConfig.getLdapMaxSharedConnections(),
                    null);

            ldapLoginDN = AuthServerConfig.getLdapLoginDN();
            ldapLoginPW = AuthServerConfig.getLdapLoginPassword();
        } catch (LDAPException ex) {
            String msg = "Failed to initialise LDAP.\n" + ex.toString();
            throw new ServerInitialisationException(msg);
        }
    }

    public static LDAPConnection getConnection() {
        LDAPConnection lc = null;
        try {
            if (FaultPoints.fiEnabled()) {
                if (FaultPoints.getInstance().isFaultPointActive("LDAP_CONNECT_FAIL")) {
                    throw new LDAPException("Connection failed", LDAPException.CONNECT_ERROR,
                            null);
                } else if (FaultPoints.getInstance().isFaultPointActive("LDAP_CONN_INTRPT")) {
                    throw new InterruptedException("Connection interrupted");
                }
            }

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

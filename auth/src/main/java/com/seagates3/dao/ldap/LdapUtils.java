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

import java.util.ArrayList;
import java.util.logging.Logger;

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.novell.ldap.LDAPSearchResults;

public class LdapUtils {
    private static final String BASEDN = "dc=s3,dc=seagate,dc=com";

    public static String getBaseDN() {
        return BASEDN;
    }

    static final Logger LOGGER = Logger.getLogger("authLog");

    /*
     * Search for an entry in LDAP and return the search results.
     */
    public static LDAPSearchResults search(String base, int scope,
            String filter, String[] attrs) {
        LDAPConnection lc;
        lc = LdapConnectionManager.getConnection();
        LDAPSearchResults ldapSearchResult = null;

        if(lc != null && lc.isConnected()) {
            try {
                ldapSearchResult = lc.search(BASEDN, scope,
                        filter, attrs , false);

            } catch (LDAPException ex) {
            }
        } else {
        }

        LdapConnectionManager.releaseConnection(lc);

        return ldapSearchResult;
    }

    /*
     * Add a new entry into LDAP.
     */
    public static Boolean add(LDAPEntry newEntry) {
        LDAPConnection lc;
        lc = LdapConnectionManager.getConnection();
        Boolean success = false;

        if(lc != null && lc.isConnected()) {
            try {
                lc.add(newEntry);
                success = true;
            } catch (LDAPException ex) {
            }
        }

        LdapConnectionManager.releaseConnection(lc);

        return success;
    }

    /*
     * Delete an entry from LDAP.
     */
    public static Boolean delete(String dn) {
        LDAPConnection lc;
        lc = LdapConnectionManager.getConnection();
        Boolean success = false;

        if(lc != null && lc.isConnected()) {
            try {
                lc.delete(dn);
                success = true;
            } catch (LDAPException ex) {
            }
        }

        LdapConnectionManager.releaseConnection(lc);

        return success;
    }

    /*
     * Modify an entry in LDAP
     */

    public static Boolean modify(String dn, LDAPModification modification) {
        LDAPConnection lc;
        lc = LdapConnectionManager.getConnection();
        Boolean success = false;

        if(lc != null && lc.isConnected()) {
            try {
                lc.modify(dn, modification);
                success = true;
            } catch (LDAPException ex) {
            }
        }

        LdapConnectionManager.releaseConnection(lc);

        return success;
    }


    public static Boolean modify(String dn, ArrayList modList) {
        LDAPConnection lc;
        lc = LdapConnectionManager.getConnection();
        Boolean success = false;

        LDAPModification[] modifications = new LDAPModification[modList.size()];
        modifications = (LDAPModification[]) modList.toArray(modifications);

        if(lc != null && lc.isConnected()) {
            try {
                lc.modify(dn, modifications);
                success = true;
            } catch (LDAPException ex) {
            }
        }

        LdapConnectionManager.releaseConnection(lc);

        return success;
    }
}

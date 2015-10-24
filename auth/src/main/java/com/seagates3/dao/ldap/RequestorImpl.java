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

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;

import com.seagates3.dao.RequestorDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Requestor;

public class RequestorImpl implements RequestorDAO {

    @Override
    public Requestor find(AccessKey accessKey) throws DataAccessException {
        Requestor requestor = new Requestor();

        requestor.setAccessKey(accessKey);

        if(accessKey.getUserId()!= null) {
            requestor.setId(accessKey.getUserId());
            String filter;
            String[] attrs = {"o", "cn"};
            LDAPSearchResults ldapResults;

            if(accessKey.getToken() == null) {
                filter = String.format("(&(uid=%s)(objectclass=iamUser))",
                        requestor.getId());
            } else {
                filter = String.format("(&(uid=%s)(objectclass=iamFedUser))",
                        requestor.getId());
            }

            try {
                ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                        LDAPConnection.SCOPE_SUB, filter, attrs);
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find requestor details.\n" + ex);
            }

            if(ldapResults.hasMore()) {
                LDAPEntry entry;
                try {
                    entry = ldapResults.next();
                } catch (LDAPException ex) {
                    throw new DataAccessException("LDAP error\n" + ex);
                }
                requestor.setAccountName(entry.getAttribute("o").getStringValue());
                requestor.setName(entry.getAttribute("cn").getStringValue());
            }
        }

        return requestor;
    }
}

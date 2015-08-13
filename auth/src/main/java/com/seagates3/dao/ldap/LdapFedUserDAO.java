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

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;

import com.seagates3.dao.FedUserDAO;
import com.seagates3.model.User;

public class LdapFedUserDAO implements FedUserDAO{

    @Override
    public User findUser(String accountName, String name) {
        User user = new User();
        user.setAccountName(accountName);
        user.setName(name);

        String[] attrs = {"id", "objectclass"};
        String ldapBase = String.format("ou=%s,%s", accountName, LdapUtils.getBaseDN());
        String filter = String.format("(cn=%s)", name);

        LDAPSearchResults ldapResults = LdapUtils.search(ldapBase,
                LDAPConnection.SCOPE_SUB, filter, attrs);
        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                user.setId(entry.getAttribute("id").getStringValue());
            } catch (LDAPException ex) {
            }
        }

        return user;
    }

    @Override
    public Boolean saveUser(User user) {
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "iamFedUser"));
        attributeSet.add( new LDAPAttribute("cn", user.getName()));
        attributeSet.add( new LDAPAttribute("ou", user.getAccountName()));
        attributeSet.add( new LDAPAttribute("path", user.getPath()));
        attributeSet.add(new LDAPAttribute("id", user.getId()));

        String dn = String.format("cn=%s,ou=%s,%s", user.getName(),
                user.getAccountName(), LdapUtils.getBaseDN());

        return LdapUtils.add(new LDAPEntry(dn, attributeSet));
    }
}

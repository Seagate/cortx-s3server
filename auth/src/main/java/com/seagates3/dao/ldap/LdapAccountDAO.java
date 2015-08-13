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
import com.novell.ldap.LDAPSearchResults;

import com.seagates3.model.Account;
import com.seagates3.dao.AccountDAO;

public class LdapAccountDAO implements AccountDAO {

    @Override
    public Account findAccount(String name) {
        Account account = new Account();
        account.setName(name);

        String[] attrs = {"ou"};
        String filter = String.format("(&(ou=%s)(objectClass=s3Account))", name);

        LDAPSearchResults ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                LDAPConnection.SCOPE_SUB, filter, attrs);

        if(ldapResults.getCount() > 0) {
            account.setAccountExists(true);
        } else {
            account.setAccountExists(false);
        }

        return account;
    }

    @Override
    public Boolean save(Account account) {
        String dn = String.format("ou=%s,%s", account.getName(), LdapUtils.getBaseDN());

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute( "objectclass", "s3Account"));
        attributeSet.add( new LDAPAttribute("ou", account.getName()));

        return LdapUtils.add(new LDAPEntry(dn, attributeSet));
    }
}

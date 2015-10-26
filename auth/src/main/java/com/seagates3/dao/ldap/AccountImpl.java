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

import com.seagates3.model.Account;
import com.seagates3.dao.AccountDAO;
import com.seagates3.exception.DataAccessException;

public class AccountImpl implements AccountDAO {

    /*
     * Fetch account details from LDAP.
     */
    @Override
    public Account find(String name) throws DataAccessException {
        Account account = new Account();
        account.setName(name);

        String[] attrs = {"o"};
        String filter = String.format("(&(o=%s)(objectClass=s3Account))", name);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to search account.\n" + ex);
        }

        if(ldapResults.hasMore()) {
            account.setAccountExists(true);
        } else {
            account.setAccountExists(false);
        }

        return account;
    }

    /*
     * Create a new entry under ou=accounts in openldap.
     * Example
     * dn: o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     *
     *
     * Create sub entries ou=user and ou=idp under the new account.
     * Example
     * dn: ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     * dn: ou=idp,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     *
     */
    @Override
    public void save(Account account) throws DataAccessException {
        String dn = String.format("o=%s,ou=accounts,%s", account.getName(), LdapUtils.getBaseDN());

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute( "objectclass", "s3Account"));
        attributeSet.add( new LDAPAttribute("o", account.getName()));

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to add new account.\n" + ex);
        }
        createUserOU(account.getName());
        createIdpOU(account.getName());
    }

    /*
     * Create sub entry ou=user for the account.
     * The dn should be in the following format
     * dn: ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    private void createUserOU(String accountName) throws DataAccessException {
        String dn = String.format("ou=users,o=%s,ou=accounts,%s",
                accountName, LdapUtils.getBaseDN());

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute( "objectclass", "organizationalunit"));
        attributeSet.add( new LDAPAttribute("ou", "users"));

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to create user ou.\n" + ex);
        }
    }

    /*
     * Create sub entry ou=idp for the account.
     * The dn should be in the following format
     * dn: ou=idp,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    private void createIdpOU(String accountName) throws DataAccessException {
        String dn = String.format("ou=idp,o=%s,ou=accounts,%s",
                accountName, LdapUtils.getBaseDN());

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute( "objectclass", "organizationalunit"));
        attributeSet.add( new LDAPAttribute("ou", "idp"));

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to create idp ou.\n" + ex);
        }
    }
}

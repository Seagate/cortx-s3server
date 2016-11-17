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

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.AccountDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.fi.FaultPoints;
import com.seagates3.model.Account;
import java.util.ArrayList;

public class AccountImpl implements AccountDAO {

    @Override
    public Account findByID(String accountID) throws DataAccessException {
        Account account = new Account();
        account.setId(accountID);

        String[] attrs = {LDAPUtils.ORGANIZATIONAL_NAME,
            LDAPUtils.CANONICAL_ID};
        String filter = String.format("(&(%s=%s)(%s=%s))",
                LDAPUtils.ACCOUNT_ID, accountID, LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ACCOUNT_OBJECT_CLASS);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LDAPUtils.search(LDAPUtils.BASE_DN,
                    LDAPConnection.SCOPE_SUB, filter, attrs
            );
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to search account.\n" + ex);
        }

        if (ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                account.setName(entry.getAttribute(
                        LDAPUtils.ORGANIZATIONAL_NAME).getStringValue());
                account.setCanonicalId(entry.getAttribute(
                        LDAPUtils.CANONICAL_ID).getStringValue());
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find account details.\n" + ex);
            }
        }

        return account;
    }

    /**
     * Fetch account details from LDAP.
     *
     * @param name Account name
     * @return Account
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public Account find(String name) throws DataAccessException {
        Account account = new Account();
        account.setName(name);

        String[] attrs = {LDAPUtils.ACCOUNT_ID, LDAPUtils.CANONICAL_ID};
        String filter = String.format("(&(%s=%s)(%s=%s))",
                LDAPUtils.ORGANIZATIONAL_NAME, name, LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ACCOUNT_OBJECT_CLASS);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LDAPUtils.search(LDAPUtils.BASE_DN,
                    LDAPConnection.SCOPE_SUB, filter, attrs
            );
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to search account.\n" + ex);
        }

        if (ldapResults.hasMore()) {
            try {
                if (FaultPoints.fiEnabled() &&
                        FaultPoints.getInstance().isFaultPointActive("LDAP_GET_ATTR_FAIL")) {
                    throw new LDAPException();
                }

                LDAPEntry entry = ldapResults.next();
                account.setId(entry.getAttribute(LDAPUtils.ACCOUNT_ID).
                        getStringValue());
                account.setCanonicalId(entry.getAttribute(
                        LDAPUtils.CANONICAL_ID).getStringValue());
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find account details.\n" + ex);
            }
        }

        return account;
    }

    /*
     * fetch all accounts from database
     */
    public Account[] findAll() throws DataAccessException {
        ArrayList accounts = new ArrayList();
        Account account;
        LDAPSearchResults ldapResults;
        /*
         * search base: the starting point for search
         * example: 'ou=accounts,dc=s3,dc=seagate,dc=com'
         */
        String baseDn = String.format("%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN);
        /*
         * search filter: '(objectClass=account)'
         */
        String accountFilter = String.format("(%s=%s)",
                LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ACCOUNT_OBJECT_CLASS);
        String[] attr = {LDAPUtils.ORGANIZATIONAL_NAME, LDAPUtils.ACCOUNT_ID,
                LDAPUtils.EMAIL, LDAPUtils.CANONICAL_ID};

        try {
            ldapResults = LDAPUtils.search(baseDn, LDAPConnection.SCOPE_SUB,
                    accountFilter, attr);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to fetch accounts.\n" + ex);
        }
        while (ldapResults.hasMore()) {
            LDAPEntry ldapEntry;
            account = new Account();
            try {
                if (FaultPoints.fiEnabled() &&
                        FaultPoints.getInstance().isFaultPointActive("LDAP_GET_ATTR_FAIL")) {
                    throw new LDAPException();
                }

                ldapEntry = ldapResults.next();
            } catch (LDAPException ldapException) {
                throw new DataAccessException("Failed to read ldapEntry.\n"
                        + ldapException);
            }
            account.setName(ldapEntry.getAttribute(LDAPUtils.
                    ORGANIZATIONAL_NAME).getStringValue());
            account.setId(ldapEntry.getAttribute(LDAPUtils.
                    ACCOUNT_ID).getStringValue());
            account.setEmail(ldapEntry.getAttribute(LDAPUtils.
                    EMAIL).getStringValue());
            account.setCanonicalId(ldapEntry.getAttribute(LDAPUtils.
                    CANONICAL_ID).getStringValue());
            accounts.add(account);
        }
        Account[] accountList = new Account[accounts.size()];

        return (Account[]) accounts.toArray(accountList);
    }

    /**
     * Create a new entry under ou=accounts in openldap. Example dn:
     * o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     *
     * Create sub entries ou=user and ou=roles under the new account. dn:
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com dn:
     * ou=roles,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     *
     * @param account Account
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void save(Account account) throws DataAccessException {
        String dn = String.format("%s=%s,%s=accounts,%s",
                LDAPUtils.ORGANIZATIONAL_NAME, account.getName(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN);

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS, "Account"));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ACCOUNT_ID,
                account.getId()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ORGANIZATIONAL_NAME,
                account.getName()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.EMAIL,
                account.getEmail()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.CANONICAL_ID,
                account.getCanonicalId()));

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to add new account.\n" + ex);
        }

        createUserOU(account.getName());
        createRoleOU(account.getName());
        createGroupsOU(account.getName());
        createPolicyOU(account.getName());
    }

    /**
     * Create sub entry ou=users for the account. The dn should be in the
     * following format dn:
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    private void createUserOU(String accountName) throws DataAccessException {
        String dn = String.format("%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.USER_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, accountName,
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ORGANIZATIONAL_UNIT_CLASS));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                LDAPUtils.USER_OU));

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to create user ou.\n" + ex);
        }
    }

    /**
     * Create sub entry ou=roles for the account. The dn should be in the
     * following format dn:
     * ou=roles,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    private void createRoleOU(String accountName) throws DataAccessException {
        String dn = String.format("%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ROLE_OU,
                LDAPUtils.ORGANIZATIONAL_NAME,
                accountName, LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN
        );

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ORGANIZATIONAL_UNIT_CLASS));
        attributeSet.add(new LDAPAttribute("ou", "roles"));

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to create role ou.\n" + ex);
        }
    }

    /**
     * Create sub entry ou=policies for the account. The dn should be in the
     * following format dn:
     * ou=policies,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    private void createPolicyOU(String accountName) throws DataAccessException {
        String dn = String.format("%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.POLICY_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, accountName,
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ORGANIZATIONAL_UNIT_CLASS));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                LDAPUtils.POLICY_OU));

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to create policy ou.\n" + ex);
        }
    }

    /**
     * Create sub entry ou=groups for the account. The dn should be in the
     * following format dn:
     * ou=groups,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    private void createGroupsOU(String accountName) throws DataAccessException {
        String dn = String.format("%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.GROUP_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, accountName,
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ORGANIZATIONAL_UNIT_CLASS));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                LDAPUtils.GROUP_OU));

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("failed to create groups ou.\n" + ex);
        }
    }
}

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
import java.util.Date;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.novell.ldap.LDAPSearchResults;

import com.seagates3.dao.UserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;

public class UserImpl implements UserDAO {

    /*
     * Get the user details from LDAP.
     *
     * Search for the user under
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    @Override
    public User find(String accountName, String name) throws DataAccessException {
        User user = new User();
        user.setAccountName(accountName);
        user.setName(name);

        String[] attrs = {"uid", "path", "objectclass"};
        String ldapBase = String.format("ou=users,o=%s,ou=accounts,%s", accountName,
                LdapUtils.getBaseDN());
        String filter = String.format("(cn=%s)", name);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(ldapBase,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find user details.\n" + ex);
        }

        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                user.setId(entry.getAttribute("uid").getStringValue());
                user.setPath(entry.getAttribute("path").getStringValue());

                if(entry.getAttribute("objectclass").getStringValue().
                        compareTo("iamFedUser") == 0) {
                    user.setFederateduser(Boolean.TRUE);
                } else {
                    user.setFederateduser(Boolean.FALSE);
                }
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find user details.\n" + ex);
            }
        }

        return user;
    }

    /*
     * Get all the users with path prefix from LDAP.
     *
     * Search for the users under
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    @Override
    public User[] findAll(String accountName, String pathPrefix) throws DataAccessException {
        ArrayList users = new ArrayList();
        User user;
        Date d;

        String[] attrs = {"uid", "cn", "path", "createTimestamp"};
        String ldapBase = String.format("ou=users,o=%s,ou=accounts,%s", accountName,
                LdapUtils.getBaseDN());
        String filter = String.format("(&(path=%s*)(objectclass=iamUser))",
                pathPrefix, accountName);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(ldapBase,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find all user details.\n" + ex);
        }

        while(ldapResults.hasMore()) {
            user = new User();
            LDAPEntry entry;
            try {
                entry = ldapResults.next();
            } catch (LDAPException ex) {
                throw new DataAccessException("Ldap failure.\n" + ex);
            }
            user.setId(entry.getAttribute("uid").getStringValue());
            user.setName(entry.getAttribute("cn").getStringValue());
            user.setPath(entry.getAttribute("path").getStringValue());

            d = DateUtil.toDate(entry.getAttribute("createTimeStamp").getStringValue());
            user.setCreateDate(DateUtil.toServerResponseFormat(d));

            users.add(user);
        }

        User[] userList = new User[users.size()];
        return (User[]) users.toArray(userList);
    }

    /*
     * Delete the user from LDAP.
     *
     * Search for the users under
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    @Override
    public void delete(User user) throws DataAccessException {
        String dn = String.format("uid=%s,ou=users,o=%s,ou=accounts,%s", user.getId(),
                user.getAccountName(), LdapUtils.getBaseDN());

        try {
            LdapUtils.delete(dn);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to delete the user.\n" + ex);
        }
    }

    /*
     * Create a new entry for the user in LDAP.
     */
    @Override
    public void save(User user) throws DataAccessException {
        String objectClass = user.isFederatedUser() ? "iamFedUser" : "iamUser";

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", objectClass));
        attributeSet.add( new LDAPAttribute("cn", user.getName()));
        attributeSet.add( new LDAPAttribute("ou", user.getAccountName()));
        attributeSet.add(new LDAPAttribute("uid", user.getId()));

        if(user.getPath() != null) {
            attributeSet.add( new LDAPAttribute("path", user.getPath()));
        }

        String dn = String.format("uid=%s,ou=users,o=%s,ou=accounts,%s",
                user.getId(), user.getAccountName(), LdapUtils.getBaseDN());

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to save the user.\n" + ex);
        }
    }

    /*
     * Update the user details.
     */
    @Override
    public void update(User user, String newUserName, String newPath) throws DataAccessException {
        ArrayList modList = new ArrayList();
        LDAPAttribute attr;

        String dn = String.format("uid=%s,ou=users,o=%s,ou=accounts,dc=s3,dc=seagate,dc=com",
                user.getId(), user.getAccountName());

        if(newUserName != null) {
            attr = new LDAPAttribute("cn", newUserName);
            modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
        }

        if(newPath != null) {
            attr = new LDAPAttribute("path", newPath);
            modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
        }

        try {
            LdapUtils.modify(dn, modList);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to modify the user details.\n" + ex);
        }
    }
}

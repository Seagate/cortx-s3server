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
import com.novell.ldap.LDAPModification;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.UserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;
import java.util.ArrayList;

public class UserImpl implements UserDAO {

    /**
     * Get the user details from LDAP.
     *
     * Search for the user under
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     *
     * @param accountName Account name
     * @param name User name
     * @return User
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public User find(String accountName, String name)
            throws DataAccessException {
        User user = new User();
        user.setAccountName(accountName);
        user.setName(name);

        String[] attrs = {LDAPUtils.USER_ID, LDAPUtils.PATH, LDAPUtils.ROLE_NAME,
            LDAPUtils.OBJECT_CLASS, LDAPUtils.CREATE_TIMESTAMP};

        String userBaseDN = String.format("%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.USER_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, accountName,
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );

        String filter = String.format("(%s=%s)", LDAPUtils.COMMON_NAME, name);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LDAPUtils.search(userBaseDN,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find user details.\n" + ex);
        }

        if (ldapResults.hasMore()) {
            LDAPEntry entry;

            try {
                entry = ldapResults.next();
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find user details.\n" + ex);
            }

            user.setId(entry.getAttribute(LDAPUtils.USER_ID).getStringValue());

            String objectClass = entry.getAttribute(LDAPUtils.OBJECT_CLASS).
                    getStringValue();
            user.setUserType(objectClass);

            if (user.getUserType() == User.UserType.IAM_USER) {
                user.setPath(entry.getAttribute(LDAPUtils.PATH).
                        getStringValue());
            }

            if (user.getUserType() == User.UserType.ROLE_USER) {
                user.setRoleName(entry.getAttribute(LDAPUtils.ROLE_NAME).
                        getStringValue());
            }

            String createTime = DateUtil.toServerResponseFormat(
                    entry.getAttribute(LDAPUtils.CREATE_TIMESTAMP).
                    getStringValue());
            user.setCreateDate(createTime);

        }

        return user;
    }

    /**
     * Get all the IAM users with path prefix from LDAP.
     *
     * @param accountName Account name
     * @param pathPrefix Path prefix
     * @return List of users with given path prefix.
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public User[] findAll(String accountName, String pathPrefix)
            throws DataAccessException {
        ArrayList users = new ArrayList();
        User user;

        String[] attrs = {LDAPUtils.USER_ID, LDAPUtils.COMMON_NAME,
            LDAPUtils.PATH, LDAPUtils.CREATE_TIMESTAMP};
        String userBaseDN = String.format("%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.USER_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, accountName,
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );
        String filter = String.format("(&(%s=%s*)(%s=%s))",
                LDAPUtils.PATH, pathPrefix, LDAPUtils.OBJECT_CLASS,
                LDAPUtils.IAMUSER_OBJECT_CLASS, LDAPUtils.IAMUSER_OBJECT_CLASS
        );

        LDAPSearchResults ldapResults;

        try {
            ldapResults = LDAPUtils.search(userBaseDN,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find all user details.\n" + ex);
        }

        while (ldapResults.hasMore()) {
            user = new User();
            LDAPEntry entry;
            try {
                entry = ldapResults.next();
            } catch (LDAPException ex) {
                throw new DataAccessException("Ldap failure.\n" + ex);
            }
            user.setId(entry.getAttribute(LDAPUtils.USER_ID).getStringValue());
            user.setName(entry.getAttribute(LDAPUtils.COMMON_NAME).getStringValue());
            user.setPath(entry.getAttribute(LDAPUtils.PATH).getStringValue());
            user.setAccountName(accountName);
            user.setUserType(User.UserType.IAM_USER);

            String createTime = DateUtil.toServerResponseFormat(
                    entry.getAttribute(LDAPUtils.CREATE_TIMESTAMP).getStringValue());
            user.setCreateDate(createTime);

            users.add(user);
        }

        User[] userList = new User[users.size()];
        return (User[]) users.toArray(userList);
    }

    /**
     * Delete the user from LDAP.
     *
     * @param user User
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void delete(User user) throws DataAccessException {
        String dn = String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.USER_ID, user.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.USER_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, user.getAccountName(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN);

        try {
            LDAPUtils.delete(dn);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to delete the user.\n" + ex);
        }
    }

    /**
     * Create a new entry for the user in LDAP.
     *
     * @param user User
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void save(User user) throws DataAccessException {
        String objectClass = user.getUserType().toString().toLowerCase();

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS, objectClass));
        attributeSet.add(new LDAPAttribute(LDAPUtils.COMMON_NAME, user.getName()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.USER_ID, user.getId()));

        if (user.getUserType() == User.UserType.IAM_USER) {
            attributeSet.add(new LDAPAttribute(LDAPUtils.PATH, user.getPath()));
        }

        if (user.getUserType() == User.UserType.ROLE_USER) {
            attributeSet.add(new LDAPAttribute(LDAPUtils.ROLE_NAME,
                    user.getRoleName()
            ));
        }

        String dn = String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.USER_ID, user.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.USER_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, user.getAccountName(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to save the user.\n" + ex);
        }
    }

    /**
     * Update the existing user details.
     *
     * @param user User
     * @param newUserName New User name
     * @param newPath New User Path
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void update(User user, String newUserName, String newPath)
            throws DataAccessException {
        ArrayList modList = new ArrayList();
        LDAPAttribute attr;

        String dn = String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s",
                LDAPUtils.USER_ID, user.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.USER_OU,
                LDAPUtils.ORGANIZATIONAL_NAME, user.getAccountName(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                LDAPUtils.BASE_DN
        );

        if (newUserName != null) {
            attr = new LDAPAttribute(LDAPUtils.COMMON_NAME, newUserName);
            modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
        }

        if (newPath != null) {
            attr = new LDAPAttribute(LDAPUtils.PATH, newPath);
            modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
        }

        try {
            LDAPUtils.modify(dn, modList);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to modify the user details.\n" + ex);
        }
    }
}

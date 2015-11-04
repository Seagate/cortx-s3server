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
 * Original creation date: 31-Oct-2015
 */

package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.RoleDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Role;
import com.seagates3.util.DateUtil;
import java.util.ArrayList;

public class RoleImpl implements RoleDAO {

    /*
     * Get the role from LDAP.
     *
     * Search for the role under
     * ou=roles,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    @Override
    public Role find(String accountName, String roleName) throws DataAccessException {
        Role role = new Role();
        role.setAccountName(accountName);
        role.setName(roleName);

        String[] attrs = {"rolePolicyDoc", "path", "createTimestamp"};
        String ldapBase = String.format("ou=roles,o=%s,ou=accounts,%s", accountName,
                LdapUtils.getBaseDN());
        String filter = String.format("(rolename=%s)", roleName);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(ldapBase,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find the role.\n" + ex);
        }

        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                role.setPath(entry.getAttribute("path").getStringValue());
                role.setRolePolicyDoc(entry.getAttribute("rolePolicyDoc").getStringValue());

                String createTime = DateUtil.toServerResponseFormat(
                        entry.getAttribute("createTimeStamp").getStringValue());
                role.setCreateDate(createTime);
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find user details.\n" + ex);
            }
        }

        return role;
    }

    /*
     * Get all the roles with path prefix from LDAP.
     */
    @Override
    public Role[] findAll(String accountName, String pathPrefix) throws DataAccessException {
        ArrayList roles = new ArrayList();
        Role role;

        String[] attrs = {"rolename", "rolePolicyDoc", "path", "createTimestamp"};
        String ldapBase = String.format("ou=roles,o=%s,ou=accounts,%s", accountName,
                LdapUtils.getBaseDN());
        String filter = String.format("(&(path=%s*)(objectclass=role))",
                pathPrefix, accountName);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(ldapBase,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find all roles.\n" + ex);
        }

        while(ldapResults.hasMore()) {
            role = new Role();
            LDAPEntry entry;
            try {
                entry = ldapResults.next();
            } catch (LDAPException ex) {
                throw new DataAccessException("Ldap failure.\n" + ex);
            }
            role.setAccountName(accountName);
            role.setName(entry.getAttribute("rolename").getStringValue());
            role.setPath(entry.getAttribute("path").getStringValue());
            role.setRolePolicyDoc(entry.getAttribute("rolePolicyDoc").getStringValue());

            String createTime = DateUtil.toServerResponseFormat(
                        entry.getAttribute("createTimeStamp").getStringValue());
            role.setCreateDate(createTime);

            roles.add(role);
        }

        Role[] roleList = new Role[roles.size()];
        return (Role[]) roles.toArray(roleList);
    }

    /*
     * Delete the role from LDAP.
     */
    @Override
    public void delete(Role role) throws DataAccessException {
        String dn = String.format("rolename=%s,ou=roles,o=%s,ou=accounts,%s", role.getName(),
                role.getAccountName(), LdapUtils.getBaseDN());

        try {
            LdapUtils.delete(dn);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to delete the role.\n" + ex);
        }
    }

    /*
     * Create a new entry for the role in LDAP.
     */
    @Override
    public void save(Role role) throws DataAccessException {
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "role"));
        attributeSet.add( new LDAPAttribute("rolename", role.getName()));
        attributeSet.add( new LDAPAttribute("rolepolicydoc", role.getRolePolicyDoc()));
        attributeSet.add( new LDAPAttribute("path", role.getPath()));

        String dn = String.format("rolename=%s,ou=roles,o=%s,ou=accounts,%s",
                role.getName(), role.getAccountName(), LdapUtils.getBaseDN());

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to create role.\n" + ex);
        }
    }
}

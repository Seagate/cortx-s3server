/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.FedUserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;

public class FedUserImpl implements FedUserDAO {

    /*
     * Get the federated user details from LDAP.
     *
     * Search for the user under
     * ou=users,o=<account name>,ou=accounts,dc=s3,dc=seagate,dc=com
     */
    @Override
    public User find(String accountName, String name) throws DataAccessException {
        User user = new User();
        user.setAccountName(accountName);
        user.setName(name);

        String[] attrs = {"id", "objectclass"};
        String ldapBase = String.format("ou=users,o=%s,ou=accounts,%s",
                accountName, LDAPUtils.getBaseDN());
        String filter = String.format("(cn=%s)", name);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LDAPUtils.search(ldapBase,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find federated user details.\n" + ex);
        }
        if (ldapResults != null && ldapResults.hasMore()) {
            LDAPEntry entry;
            try {
                entry = ldapResults.next();
            } catch (LDAPException ex) {
                throw new DataAccessException("LDAP failure.\n" + ex);
            }
            user.setId(entry.getAttribute("id").getStringValue());
        }

        return user;
    }

    /*
     * Create a new entry for the user in LDAP.
     */
    @Override
    public void save(User user) throws DataAccessException {
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute("objectclass", "iamFedUser"));
        attributeSet.add(new LDAPAttribute("cn", user.getName()));
        attributeSet.add(new LDAPAttribute("ou", user.getAccountName()));
        attributeSet.add(new LDAPAttribute("path", user.getPath()));
        attributeSet.add(new LDAPAttribute("id", user.getId()));

        String dn = String.format("id=%s,ou=users,o=%s,ou=accounts,%s",
                user.getId(), user.getAccountName(), LDAPUtils.getBaseDN());

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to save federated user.\n" + ex);
        }
    }
}

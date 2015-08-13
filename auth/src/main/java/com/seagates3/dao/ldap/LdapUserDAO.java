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
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;

public class LdapUserDAO implements UserDAO {

    @Override
    public User findUser(String accountName, String name) {
        User user = new User();
        user.setAccountName(accountName);
        user.setName(name);

        String[] attrs = {"id", "path", "objectclass"};
        String ldapBase = String.format("ou=%s,%s", accountName, LdapUtils.getBaseDN());
        String filter = String.format("(cn=%s)", name);

        LDAPSearchResults ldapResults = LdapUtils.search(ldapBase,
                LDAPConnection.SCOPE_SUB, filter, attrs);
        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                user.setId(entry.getAttribute("id").getStringValue());
                user.setPath(entry.getAttribute("path").getStringValue());

                if(entry.getAttribute("objectclass").getStringValue().
                        compareTo("iamFedUser") == 0) {
                    user.setFederateduser(Boolean.TRUE);
                } else {
                    user.setFederateduser(Boolean.FALSE);
                }
            } catch (LDAPException ex) {
            }
        }

        return user;
    }

    @Override
    public User[] findUsers(String accountName, String pathPrefix) {
        ArrayList users = new ArrayList();
        User user;
        Date d;

        String[] attrs = {"id", "cn", "path", "createTimestamp"};
        String filter = String.format("(&(path=%s*)(ou=%s)(objectclass=iamUser))",
                pathPrefix, accountName);

        LDAPSearchResults ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                LDAPConnection.SCOPE_SUB, filter, attrs);

        while(ldapResults.hasMore()) {
            user = new User();
            try {
                LDAPEntry entry = ldapResults.next();
                user.setId(entry.getAttribute("id").getStringValue());
                user.setName(entry.getAttribute("cn").getStringValue());
                user.setPath(entry.getAttribute("path").getStringValue());

                d = DateUtil.toDate(entry.getAttribute("createTimeStamp").getStringValue());
                user.setCreateDate(DateUtil.toServerResponseFormat(d));

                users.add(user);
            } catch (LDAPException ex) {
                return null;
            }
        }

        User[] userList = new User[users.size()];
        return (User[]) users.toArray(userList);
    }

    @Override
    public boolean deleteUser(User user) {
        String dn = String.format("id=%s,ou=%s,%s", user.getId(),
                user.getAccountName(), LdapUtils.getBaseDN());

        return LdapUtils.delete(dn);
    }

    @Override
    public Boolean saveUser(User user) {
        String objectClass = user.isFederatedUser() ? "iamFedUser" : "iamUser";

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", objectClass));
        attributeSet.add( new LDAPAttribute("cn", user.getName()));
        attributeSet.add( new LDAPAttribute("ou", user.getAccountName()));
        attributeSet.add(new LDAPAttribute("id", user.getId()));

        if(user.getPath() != null) {
            attributeSet.add( new LDAPAttribute("path", user.getPath()));
        }

        String dn = String.format("id=%s,ou=%s,%s", user.getId(), user.getAccountName(),
                LdapUtils.getBaseDN());

        return LdapUtils.add(new LDAPEntry(dn, attributeSet));
    }

    @Override
    public Boolean updateUser(User user, String newUserName, String newPath) {
        ArrayList modList = new ArrayList();
        LDAPAttribute attr;

        String dn = String.format("id=%s,ou=%s,dc=s3,dc=seagate,dc=com",
                user.getId(), user.getAccountName());

        if(newUserName != null) {
            attr = new LDAPAttribute("cn", newUserName);
            modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
        }

        if(newPath != null) {
            attr = new LDAPAttribute("path", newPath);
            modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
        }

        return LdapUtils.modify(dn, modList);
    }
}

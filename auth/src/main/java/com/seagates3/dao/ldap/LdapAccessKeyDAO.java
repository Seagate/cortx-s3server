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


import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKeyStatus;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;

public class LdapAccessKeyDAO implements AccessKeyDAO{

    @Override
    public AccessKey findAccessKey(String accessKeyId) {
        AccessKey accessKey = new AccessKey();
        accessKey.setAccessKeyId(accessKeyId);

        String[] attrs = {"id", "sk", "status", "createTimestamp"};
        String filter = String.format("(&(ak=%s)(objectclass=accessKey))",
                accessKeyId);

        LDAPSearchResults ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                LDAPConnection.SCOPE_SUB, filter, attrs);
        Date d;
        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                accessKey.setUserId(entry.getAttribute("id").getStringValue());
                accessKey.setSecretKey(entry.getAttribute("sk").getStringValue());
                AccessKeyStatus status = AccessKeyStatus.valueOf(
                        entry.getAttribute("status").getStringValue().toUpperCase());
                accessKey.setStatus(status);

                d = DateUtil.toDate(entry.getAttribute("createTimestamp").getStringValue());
                accessKey.setCreateDate(DateUtil.toServerResponseFormat(d));
            } catch (LDAPException ex) {
            }
        }

        return accessKey;
    }

    @Override
    public AccessKey[] findUserAccessKeys(User user) {
        ArrayList accessKeys = new ArrayList();
        AccessKey accessKey;

        String[] attrs = {"ak", "status", "createTimestamp"};
        String filter = String.format("(&(id=%s)(objectclass=accessKey))",
                user.getId());

        LDAPSearchResults ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                LDAPConnection.SCOPE_SUB, filter, attrs);
        Date d;
        AccessKeyStatus accessKeystatus;
        while(ldapResults.hasMore()) {
            accessKey = new AccessKey();
            try {
                LDAPEntry entry = ldapResults.next();
                accessKey.setAccessKeyId(entry.getAttribute("ak").getStringValue());

                accessKeystatus = AccessKeyStatus.valueOf(
                        entry.getAttribute("status").getStringValue().toUpperCase());
                accessKey.setStatus(accessKeystatus);

                d = DateUtil.toDate(entry.getAttribute("createTimestamp").getStringValue());
                accessKey.setCreateDate(DateUtil.toServerResponseFormat(d));

                accessKeys.add(accessKey);
            } catch (LDAPException ex) {
            }
        }

        AccessKey[] accessKeyList = new AccessKey[accessKeys.size()];
        return (AccessKey[]) accessKeys.toArray(accessKeyList);
    }

    @Override
    public Boolean hasAccessKeys(String userId) {
        return getCount(userId) > 0;
    }

    @Override
    public int getCount(String userId) {
        String filter;
        String[] attrs = new String[] {"ak", "status"};

        filter = String.format("(&(id=%s)(objectclass=accessKey))", userId);

        LDAPSearchResults ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                LDAPConnection.SCOPE_SUB, filter, attrs);

        return ldapResults.getCount();
    }

    @Override
    public Boolean delete(AccessKey accessKey) {
        String dn = String.format("ak=%s,ou=accesskeys,dc=s3,dc=seagate,dc=com",
                accessKey.getAccessKeyId());

        return LdapUtils.delete(dn);
    }

    @Override
    public Boolean save(AccessKey accessKey) {
        if(accessKey.getToken() != null) {
            return saveFedAccessKey(accessKey);
        } else {
            return saveAccessKey(accessKey);
        }
    }

    @Override
    public Boolean updateAccessKey(AccessKey accessKey, String newStatus) {
        String dn = String.format("ak=%s,ou=accesskeys,dc=s3,dc=seagate,dc=com",
                accessKey.getAccessKeyId());
        LDAPAttribute attr = new LDAPAttribute("status", newStatus);
        LDAPModification modify = new LDAPModification(LDAPModification.REPLACE, attr);

        return LdapUtils.modify(dn, modify);
    }

    private Boolean saveAccessKey(AccessKey accessKey) {
        String dn;

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "accessKey"));
        attributeSet.add( new LDAPAttribute("id", accessKey.getUserId()));
        attributeSet.add( new LDAPAttribute("ak", accessKey.getAccessKeyId()));
        attributeSet.add( new LDAPAttribute("sk", accessKey.getSecretKey()));
        attributeSet.add( new LDAPAttribute("status", accessKey.getStatus()));

        dn = String.format("ak=%s,ou=accesskeys,%s", accessKey.getAccessKeyId(),
                LdapUtils.getBaseDN());

        return LdapUtils.add(new LDAPEntry(dn, attributeSet));
    }

    private Boolean saveFedAccessKey(AccessKey accessKey) {
        String dn;

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "fedaccessKey"));
        attributeSet.add( new LDAPAttribute("id", accessKey.getUserId()));
        attributeSet.add( new LDAPAttribute("ak", accessKey.getAccessKeyId()));
        attributeSet.add( new LDAPAttribute("sk", accessKey.getSecretKey()));
        attributeSet.add( new LDAPAttribute("token", accessKey.getToken()));
        attributeSet.add( new LDAPAttribute("expiry", accessKey.getExpiry()));
        attributeSet.add( new LDAPAttribute("status", accessKey.getStatus()));

        dn = String.format("ak=%s,ou=accesskeys,%s", accessKey.getAccessKeyId(),
                LdapUtils.getBaseDN());

        return LdapUtils.add(new LDAPEntry(dn, attributeSet));
    }
}

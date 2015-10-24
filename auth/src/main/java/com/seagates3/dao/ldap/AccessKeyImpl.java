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
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKey.AccessKeyStatus;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;

public class AccessKeyImpl implements AccessKeyDAO{

    /*
     * Get the access key details from the database.
     */
    @Override
    public AccessKey find(String accessKeyId) throws DataAccessException {
        AccessKey accessKey = new AccessKey();
        accessKey.setAccessKeyId(accessKeyId);

        String[] attrs = {"uid", "sk", "status", "createTimestamp"};
        String filter = String.format("(&(ak=%s)(objectclass=accessKey))",
                accessKeyId);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Access key find failed.\n" + ex);
        }

        Date d;
        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                accessKey.setUserId(entry.getAttribute("uid").getStringValue());
                accessKey.setSecretKey(entry.getAttribute("sk").getStringValue());
                AccessKeyStatus status = AccessKeyStatus.valueOf(
                        entry.getAttribute("status").getStringValue().toUpperCase());
                accessKey.setStatus(status);

                d = DateUtil.toDate(entry.getAttribute("createTimestamp").getStringValue());
                accessKey.setCreateDate(DateUtil.toServerResponseFormat(d));
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to update AccessKey.\n" + ex);
            }
        }

        return accessKey;
    }

    /*
     * Get the access key belonging to the user from LDAP.
     */
    @Override
    public AccessKey[] findAll(User user) throws DataAccessException {
        ArrayList accessKeys = new ArrayList();
        AccessKey accessKey;

        String[] attrs = {"ak", "status", "createTimestamp"};
        String filter = String.format("(&(uid=%s)(objectclass=accessKey))",
                user.getId());

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to searcf access keys" + ex);
        }

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
                throw new DataAccessException("Access key find failed.\n" + ex);
            }
        }

        AccessKey[] accessKeyList = new AccessKey[accessKeys.size()];
        return (AccessKey[]) accessKeys.toArray(accessKeyList);
    }

    /*
     * Return true if the user has an access key.
     */
    @Override
    public Boolean hasAccessKeys(String userId) throws DataAccessException {
        return getCount(userId) > 0;
    }

    /*
     * Return the no of access keys which a user has.
     */
    @Override
    public int getCount(String userId) throws DataAccessException {
        try {
            String filter;
            String[] attrs = new String[] {"ak", "status"};

            filter = String.format("(&(uid=%s)(objectclass=accessKey))", userId);

            LDAPSearchResults ldapResults;
            ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                    LDAPConnection.SCOPE_SUB, filter, attrs);

            return ldapResults.getCount();
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to get the count of user access keys" + ex);
        }
    }

    /*
     * Delete the user access key.
     */
    @Override
    public void delete(AccessKey accessKey) throws DataAccessException {
        try {
            String dn = String.format("ak=%s,ou=accesskeys,dc=s3,dc=seagate,dc=com",
                    accessKey.getAccessKeyId());

            LdapUtils.delete(dn);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to delete access key" + ex);
        }
    }

    /*
     * Save the user access key.
     */
    @Override
    public void save(AccessKey accessKey) throws DataAccessException {
        if(accessKey.getToken() != null) {
            saveFedAccessKey(accessKey);
        } else {
            saveAccessKey(accessKey);
        }
    }

    /*
     * Return the no of access keys which a user has.
     */
    @Override
    public void update(AccessKey accessKey, String newStatus) throws DataAccessException {
        String dn = String.format("ak=%s,ou=accesskeys,dc=s3,dc=seagate,dc=com",
                accessKey.getAccessKeyId());
        LDAPAttribute attr = new LDAPAttribute("status", newStatus);
        LDAPModification modify = new LDAPModification(LDAPModification.REPLACE, attr);

        try {
            LdapUtils.modify(dn, modify);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to update the access key" + ex);
        }
    }

    /*
     * Save the access key in LDAP.
     */
    private void saveAccessKey(AccessKey accessKey) throws DataAccessException {
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "accessKey"));
        attributeSet.add( new LDAPAttribute("uid", accessKey.getUserId()));
        attributeSet.add( new LDAPAttribute("ak", accessKey.getAccessKeyId()));
        attributeSet.add( new LDAPAttribute("sk", accessKey.getSecretKey()));
        attributeSet.add( new LDAPAttribute("status", accessKey.getStatus()));

        String dn = String.format("ak=%s,ou=accesskeys,%s", accessKey.getAccessKeyId(),
                LdapUtils.getBaseDN());

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to save access key" + ex);
        }
    }

    /*
     * Save the federated access key in LDAP.
     */
    private void saveFedAccessKey(AccessKey accessKey) throws DataAccessException {
        String dn;

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "fedaccessKey"));
        attributeSet.add( new LDAPAttribute("uid", accessKey.getUserId()));
        attributeSet.add( new LDAPAttribute("ak", accessKey.getAccessKeyId()));
        attributeSet.add( new LDAPAttribute("sk", accessKey.getSecretKey()));
        attributeSet.add( new LDAPAttribute("token", accessKey.getToken()));
        attributeSet.add( new LDAPAttribute("expiry", accessKey.getExpiry()));
        attributeSet.add( new LDAPAttribute("status", accessKey.getStatus()));

        dn = String.format("ak=%s,ou=accesskeys,%s", accessKey.getAccessKeyId(),
                LdapUtils.getBaseDN());

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to save federated access key" + ex);
        }
    }
}

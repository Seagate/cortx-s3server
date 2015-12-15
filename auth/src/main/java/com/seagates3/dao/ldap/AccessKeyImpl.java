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
import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKey.AccessKeyStatus;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;
import java.util.ArrayList;

public class AccessKeyImpl implements AccessKeyDAO {

    /*
     * Get the access key details from the database.
     * Search for access key and federated access key.
     */
    @Override
    public AccessKey find(String accessKeyId) throws DataAccessException {
        AccessKey accessKey = new AccessKey();
        accessKey.setAccessKeyId(accessKeyId);

        /**
         * TODO - Convert the LDAP attribute into final strings. Use LDAPHelper
         * class for this.
         */
        String[] attrs = {"id", "sk", "exp", "token", "status",
            "createTimestamp", "objectclass"};
        String filter = String.format("ak=%s",
                accessKeyId);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Access key find failed.\n" + ex);
        }

        if (ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                accessKey.setUserId(entry.getAttribute("id").getStringValue());
                accessKey.setSecretKey(entry.getAttribute("sk").getStringValue());
                AccessKeyStatus status = AccessKeyStatus.valueOf(
                        entry.getAttribute("status").getStringValue().toUpperCase());
                accessKey.setStatus(status);

                String createTime = DateUtil.toServerResponseFormat(
                        entry.getAttribute("createTimestamp").getStringValue());
                accessKey.setCreateDate(createTime);

                String objectClass = entry.getAttribute("objectclass").getStringValue();
                if (objectClass.equalsIgnoreCase("fedaccesskey")) {
                    String expiry = DateUtil.toServerResponseFormat(
                            entry.getAttribute("exp").getStringValue());

                    accessKey.setExpiry(expiry);
                    accessKey.setToken(entry.getAttribute("token").getStringValue());
                }
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to update AccessKey.\n" + ex);
            }
        }

        return accessKey;
    }

    /*
     * Get the access key belonging to the user from LDAP.
     * Federated access keys belonging to the user should not be displayed.
     */
    @Override
    public AccessKey[] findAll(User user) throws DataAccessException {
        ArrayList accessKeys = new ArrayList();
        AccessKey accessKey;

        String[] attrs = {"ak", "status", "createTimestamp"};
        String filter = String.format("(&(id=%s)(objectclass=accessKey))",
                user.getId());

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(LdapUtils.getBaseDN(),
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to searcf access keys" + ex);
        }

        AccessKeyStatus accessKeystatus;
        while (ldapResults.hasMore()) {
            accessKey = new AccessKey();
            try {
                LDAPEntry entry = ldapResults.next();
                accessKey.setAccessKeyId(entry.getAttribute("ak").getStringValue());

                accessKeystatus = AccessKeyStatus.valueOf(
                        entry.getAttribute("status").getStringValue().toUpperCase());
                accessKey.setStatus(accessKeystatus);

                String createTime = DateUtil.toServerResponseFormat(
                        entry.getAttribute("createTimestamp").getStringValue());
                accessKey.setCreateDate(createTime);

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
            String[] attrs = new String[]{"ak"};
            int count = 0;

            filter = String.format("(&(id=%s)(objectclass=accessKey))", userId);
            String baseDN = String.format("ou=accesskeys,%s", LdapUtils.getBaseDN());

            LDAPSearchResults ldapResults;
            ldapResults = LdapUtils.search(baseDN, LDAPConnection.SCOPE_SUB, filter, attrs);

            /**
             * TODO - Explore the getcount method available in Novell LDAP with
             * async LDAP search queries.
             */
            while (ldapResults.hasMore()) {
                try {
                    count++;
                    ldapResults.next();
                } catch (LDAPException ex) {
                    throw new DataAccessException("Access key find failed.\n" + ex);
                }
            }

            return count;
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
        if (accessKey.getToken() != null) {
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
        attributeSet.add(new LDAPAttribute("objectclass", "accessKey"));
        attributeSet.add(new LDAPAttribute("id", accessKey.getUserId()));
        attributeSet.add(new LDAPAttribute("ak", accessKey.getAccessKeyId()));
        attributeSet.add(new LDAPAttribute("sk", accessKey.getSecretKey()));
        attributeSet.add(new LDAPAttribute("status", accessKey.getStatus()));

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
        String expiry = DateUtil.toLdapDate(accessKey.getExpiry());
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute("objectclass", "fedaccessKey"));
        attributeSet.add(new LDAPAttribute("id", accessKey.getUserId()));
        attributeSet.add(new LDAPAttribute("ak", accessKey.getAccessKeyId()));
        attributeSet.add(new LDAPAttribute("sk", accessKey.getSecretKey()));
        attributeSet.add(new LDAPAttribute("token", accessKey.getToken()));
        attributeSet.add(new LDAPAttribute("expiry", expiry));
        attributeSet.add(new LDAPAttribute("status", accessKey.getStatus()));

        String dn = String.format("ak=%s,ou=accesskeys,%s", accessKey.getAccessKeyId(),
                LdapUtils.getBaseDN());

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to save federated access key" + ex);
        }
    }
}

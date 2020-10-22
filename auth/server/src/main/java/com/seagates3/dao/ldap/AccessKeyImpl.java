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

import java.util.ArrayList;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
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
import com.seagates3.fi.FaultPoints;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKey.AccessKeyStatus;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;

public class AccessKeyImpl implements AccessKeyDAO {

    private final Logger LOGGER =
            LoggerFactory.getLogger(AccessKeyImpl.class.getName());
    /**
     * Search the access key in LDAP.
     *
     * @param accessKeyId
     * @return
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public AccessKey find(String accessKeyId) throws DataAccessException {
        AccessKey accessKey = new AccessKey();
        accessKey.setId(accessKeyId);

        String[] attrs = {LDAPUtils.USER_ID, LDAPUtils.SECRET_KEY,
            LDAPUtils.EXPIRY, LDAPUtils.TOKEN, LDAPUtils.STATUS,
            LDAPUtils.CREATE_TIMESTAMP, LDAPUtils.OBJECT_CLASS};

        String accessKeyBaseDN = String.format("%s=accesskeys,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN
        );

        String filter = String.format("%s=%s", LDAPUtils.ACCESS_KEY_ID,
                accessKeyId);

        LDAPSearchResults ldapResults = null;
        LDAPConnection lc = null;
        try {

          lc = LdapConnectionManager.getConnection();

          if (lc != null && lc.isConnected()) {

            if (FaultPoints.fiEnabled() &&
                FaultPoints.getInstance().isFaultPointActive(
                    "LDAP_SEARCH_FAIL")) {
              throw new LDAPException();
            }

            ldapResults = lc.search(accessKeyBaseDN, LDAPConnection.SCOPE_SUB,
                                    filter, attrs, false);
          }
          if (ldapResults != null && ldapResults.hasMore()) {
            LDAPEntry entry;
            try {
              entry = ldapResults.next();
            }
            catch (LDAPException ex) {
              LOGGER.error("Failed to update Access Key.");
              throw new DataAccessException("Failed to update AccessKey.\n" +
                                            ex);
            }

            accessKey.setUserId(
                entry.getAttribute(LDAPUtils.USER_ID).getStringValue());
            accessKey.setSecretKey(
                entry.getAttribute(LDAPUtils.SECRET_KEY).getStringValue());
            AccessKeyStatus status =
                AccessKeyStatus.valueOf(entry.getAttribute(LDAPUtils.STATUS)
                                            .getStringValue()
                                            .toUpperCase());
            accessKey.setStatus(status);

            String createTime = DateUtil.toServerResponseFormat(
                entry.getAttribute(LDAPUtils.CREATE_TIMESTAMP)
                    .getStringValue());
            accessKey.setCreateDate(createTime);

            String objectClass =
                entry.getAttribute(LDAPUtils.OBJECT_CLASS).getStringValue();
            if (objectClass.equalsIgnoreCase("fedaccesskey")) {
              String expiry = DateUtil.toServerResponseFormat(
                  entry.getAttribute(LDAPUtils.EXPIRY).getStringValue());

                accessKey.setExpiry(expiry);
                accessKey.setToken(
                    entry.getAttribute(LDAPUtils.TOKEN).getStringValue());
            }
          }
          lc.abandon(ldapResults);
        }
        catch (LDAPException ex) {
          LOGGER.error("Failed to find Access Key.");
          throw new DataAccessException("Access key find failed.\n" + ex);
        }
        finally { LdapConnectionManager.releaseConnection(lc); }

        return accessKey;
    }

    @Override public AccessKey findFromToken(String secretToken)
        throws DataAccessException {
      AccessKey accKey = new AccessKey();
      accKey.setToken(secretToken);

      String[] dnattrs = {LDAPUtils.USER_ID,     LDAPUtils.SECRET_KEY,
                          LDAPUtils.EXPIRY,      LDAPUtils.ACCESS_KEY_ID,
                          LDAPUtils.STATUS,      LDAPUtils.CREATE_TIMESTAMP,
                          LDAPUtils.OBJECT_CLASS};

      String accKeyBaseDN =
          String.format("%s=accesskeys,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                        LDAPUtils.BASE_DN);

      String dnfilter = String.format("%s=%s", LDAPUtils.TOKEN, secretToken);

      LDAPSearchResults ldapResultsForToken;

        try {
          ldapResultsForToken = LDAPUtils.search(
              accKeyBaseDN, LDAPConnection.SCOPE_SUB, dnfilter, dnattrs);
        } catch (LDAPException ex) {
            LOGGER.error("Failed to find Access Key.");
            throw new DataAccessException("Access key find failed.\n" + ex);
        }

        if (ldapResultsForToken != null && ldapResultsForToken.hasMore()) {
          LDAPEntry ldapEntry;
            try {
              ldapEntry = ldapResultsForToken.next();
            } catch (LDAPException ex) {
                LOGGER.error("Failed to update AccessKey.");
                throw new DataAccessException("Failed to update AccessKey.\n"
                        + ex);
            }

            accKey.setUserId(
                ldapEntry.getAttribute(LDAPUtils.USER_ID).getStringValue());
            accKey.setSecretKey(
                ldapEntry.getAttribute(LDAPUtils.SECRET_KEY).getStringValue());
            AccessKeyStatus status =
                AccessKeyStatus.valueOf(ldapEntry.getAttribute(LDAPUtils.STATUS)
                                            .getStringValue()
                                            .toUpperCase());
            accKey.setStatus(status);

            String keyCreateTime = DateUtil.toServerResponseFormat(
                ldapEntry.getAttribute(LDAPUtils.CREATE_TIMESTAMP)
                    .getStringValue());
            accKey.setCreateDate(keyCreateTime);

            String keyObjectClass =
                ldapEntry.getAttribute(LDAPUtils.OBJECT_CLASS).getStringValue();

            if (keyObjectClass.equalsIgnoreCase("fedaccesskey")) {
              String expiry = DateUtil.toServerResponseFormat(
                  ldapEntry.getAttribute(LDAPUtils.EXPIRY).getStringValue());

              accKey.setExpiry(expiry);
              accKey.setId(ldapEntry.getAttribute(LDAPUtils.ACCESS_KEY_ID)
                               .getStringValue());
            }
        }

        return accKey;
    }

    /**
     * Get the access key belonging to the user from LDAP.
     *
     * @param user User
     * @return AccessKey List
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public AccessKey[] findAll(User user) throws DataAccessException {
      return find(user, true);
    }

    /**
     * Get the permanent access key belonging to the user from LDAP. Federated
     *access keys
     * belonging to the user should not be displayed.
     *
     * @param user User
     * @return AccessKey List
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override public AccessKey[] findAllPermanent(User user)
        throws DataAccessException {
      return find(user, false);
    }

    /**
     * Return true if the user has an access key.
     *
     * @param userId User ID.
     * @return True if user has access keys.
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public Boolean hasAccessKeys(String userId) throws DataAccessException {
        return getCount(userId) > 0;
    }

    /**
     * Return the no of access keys which a user has.
     *
     * @param userId User ID.
     * @return no of access keys.
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public int getCount(String userId) throws DataAccessException {
      String[] attrs = new String[]{LDAPUtils.ACCESS_KEY_ID, LDAPUtils.EXPIRY};
        int count = 0;

        String filter = String.format("(&(%s=%s)(%s=%s))",
                LDAPUtils.USER_ID, userId, LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ACCESS_KEY_OBJECT_CLASS);

        String accessKeyBaseDN = String.format("%s=accesskeys,%s",
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN);

        LDAPSearchResults ldapResults;
        try {

            ldapResults = LDAPUtils.search(accessKeyBaseDN,
                    LDAPConnection.SCOPE_SUB, filter, attrs);

            /**
             * TODO - Replace this iteration with existing getCount method if
             * available.
             */
            if (ldapResults != null) {
            while (ldapResults.hasMore()) {
              LDAPEntry entry = ldapResults.next();
              boolean temporary_credentials = true;
              String access_key_id = "", expiry_time = "";
              try {
                access_key_id = entry.getAttribute(LDAPUtils.ACCESS_KEY_ID)
                                    .getStringValue();
                expiry_time =
                    entry.getAttribute(LDAPUtils.EXPIRY).getStringValue();
              }
              catch (Exception e) {
                temporary_credentials = false;
                LOGGER.debug("Failed to get time to expire for access_key" +
                             access_key_id);
                LOGGER.error(e.getMessage());
              }
              if (!temporary_credentials) {
                count++;
              }
            }
            }
        } catch (LDAPException ex) {
            LOGGER.error("Failed to get the count of user access keys"
                    + " for user id :" + userId);
            throw new DataAccessException("Failed to get the count of user "
                    + "access keys" + ex);
        }
        return count;
    }

    /**
     * Delete the user access key.
     *
     * @param accessKey AccessKey
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void delete(AccessKey accessKey) throws DataAccessException {
        String dn = String.format("%s=%s,%s=accesskeys,%s",
                LDAPUtils.ACCESS_KEY_ID, accessKey.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN);
        try {
            LDAPUtils.delete(dn);
        } catch (LDAPException ex) {
            LOGGER.error("Failed to delete access key.");
            throw new DataAccessException("Failed to delete access key" + ex);
        }
    }

    /**
     * Save the access key.
     *
     * @param accessKey AccessKey
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void save(AccessKey accessKey) throws DataAccessException {
        if (accessKey.getToken() != null) {
            saveFedAccessKey(accessKey);
        } else {
            saveAccessKey(accessKey);
        }
        try {
          // Added delay so that newly created keys are replicated in ldap
          Thread.sleep(500);
        }
        catch (InterruptedException e) {
          LOGGER.error("Exception occurred while saving access key", e);
          Thread.currentThread().interrupt();
        }
    }

    /**
     * Update the access key of the user.
     *
     * @param accessKey AccessKey
     * @param newStatus Updated Status
     * @throws com.seagates3.exception.DataAccessException
     */
    @Override
    public void update(AccessKey accessKey, String newStatus)
            throws DataAccessException {

        String dn = String.format("%s=%s,%s=accesskeys,%s",
                LDAPUtils.ACCESS_KEY_ID, accessKey.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN
        );

        LDAPAttribute attr = new LDAPAttribute("status", newStatus);
        LDAPModification modify = new LDAPModification(LDAPModification.REPLACE,
                attr);

        try {
            LDAPUtils.modify(dn, modify);
        } catch (LDAPException ex) {
            LOGGER.error("Failed to update the access key of userId: "
                                             + accessKey.getUserId());
            throw new DataAccessException("Failed to update the access key" + ex);
        }
    }

    /**
     * Save the access key in LDAP.
     */
    private void saveAccessKey(AccessKey accessKey) throws DataAccessException {
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                LDAPUtils.ACCESS_KEY_OBJECT_CLASS));
        attributeSet.add(new LDAPAttribute(LDAPUtils.USER_ID, accessKey.getUserId()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ACCESS_KEY_ID,
                accessKey.getId()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.SECRET_KEY,
                accessKey.getSecretKey()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.STATUS,
                accessKey.getStatus()));

        String dn = String.format("%s=%s,%s=accesskeys,%s",
                LDAPUtils.ACCESS_KEY_ID, accessKey.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN
        );

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            LOGGER.error("Failed to save access key of userId:"
                                        + accessKey.getUserId());
            throw new DataAccessException("Failed to save access key" + ex);
        }
    }

    /**
     * Save the federated access key in LDAP.
     */
    private void saveFedAccessKey(AccessKey accessKey)
            throws DataAccessException {
        String expiry = DateUtil.toLdapDate(accessKey.getExpiry());
        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add(new LDAPAttribute(LDAPUtils.OBJECT_CLASS,
                LDAPUtils.FED_ACCESS_KEY_OBJECT_CLASS));
        attributeSet.add(new LDAPAttribute(LDAPUtils.USER_ID,
                accessKey.getUserId()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.ACCESS_KEY_ID,
                accessKey.getId()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.SECRET_KEY,
                accessKey.getSecretKey()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.TOKEN,
                accessKey.getToken()));
        attributeSet.add(new LDAPAttribute(LDAPUtils.EXPIRY,
                expiry));
        attributeSet.add(new LDAPAttribute(LDAPUtils.STATUS,
                accessKey.getStatus()));

        String dn = String.format("%s=%s,%s=accesskeys,%s",
                LDAPUtils.ACCESS_KEY_ID, accessKey.getId(),
                LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.BASE_DN
        );

        try {
            LDAPUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            LOGGER.error("Failed to save federated access key.");
            throw new DataAccessException("Failed to save federated access key" + ex);
        }
    }

   public
    AccessKey[] find(User user, boolean fetchAll) throws DataAccessException {
      ArrayList accessKeys = new ArrayList();
      AccessKey accessKey;

      String[] attrs = {LDAPUtils.ACCESS_KEY_ID,    LDAPUtils.STATUS,
                        LDAPUtils.CREATE_TIMESTAMP, LDAPUtils.TOKEN};

      String accessKeyBaseDN =
          String.format("%s=accesskeys,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                        LDAPUtils.BASE_DN);

      String filter = String.format("(&(%s=%s)(%s=%s))", LDAPUtils.USER_ID,
                                    user.getId(), LDAPUtils.OBJECT_CLASS,
                                    LDAPUtils.ACCESS_KEY_OBJECT_CLASS);

      LDAPSearchResults ldapResults;
      try {
        ldapResults = LDAPUtils.search(accessKeyBaseDN,
                                       LDAPConnection.SCOPE_SUB, filter, attrs);
      }
      catch (LDAPException ex) {
        LOGGER.error("Failed to search access keys.");
        throw new DataAccessException("Failed to search access keys" + ex);
      }

      AccessKeyStatus accessKeystatus;
      LDAPEntry entry;
      int count = 0;
      if (ldapResults != null) {
        // TODO checking default search count = 500 to avoid failure. Make this
        // configurable
        while (ldapResults.hasMore() && count < 500) {
        accessKey = new AccessKey();
        try {
          entry = ldapResults.next();
        }
        catch (LDAPException ex) {
          LOGGER.error("Access key find failed.");
          throw new DataAccessException("Access key find failed.\n" + ex);
        }
        if (fetchAll || entry.getAttribute(LDAPUtils.TOKEN) == null) {

          accessKey.setId(
              entry.getAttribute(LDAPUtils.ACCESS_KEY_ID).getStringValue());
          accessKeystatus =
              AccessKeyStatus.valueOf(entry.getAttribute(LDAPUtils.STATUS)
                                          .getStringValue()
                                          .toUpperCase());
          accessKey.setStatus(accessKeystatus);

          String createTime = DateUtil.toServerResponseFormat(
              entry.getAttribute(LDAPUtils.CREATE_TIMESTAMP).getStringValue());
          accessKey.setCreateDate(createTime);

          accessKeys.add(accessKey);
        }
        count++;
      }
      }
      AccessKey[] accessKeyList = new AccessKey[accessKeys.size()];
      return (AccessKey[])accessKeys.toArray(accessKeyList);
    }
    /**
     * Below will delete expired access keys from ldap
     */
    @Override public void deleteExpiredKeys(User user)
        throws DataAccessException {

      String[] attrs = {LDAPUtils.ACCESS_KEY_ID, LDAPUtils.EXPIRY};

      String accessKeyBaseDN =
          String.format("%s=accesskeys,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                        LDAPUtils.BASE_DN);

      String filter = String.format("(&(%s=%s)(%s=%s))", LDAPUtils.USER_ID,
                                    user.getId(), LDAPUtils.OBJECT_CLASS,
                                    LDAPUtils.ACCESS_KEY_OBJECT_CLASS);

      LDAPSearchResults ldapResults;
      try {
        ldapResults = LDAPUtils.search(accessKeyBaseDN,
                                       LDAPConnection.SCOPE_SUB, filter, attrs);
      }
      catch (LDAPException ex) {
        LOGGER.error("Failed to search access keys.");
        throw new DataAccessException("Failed to search access keys" + ex);
      }
      AccessKey accessKey = new AccessKey();
      LDAPEntry entry;
      int count = 0;
      if (ldapResults != null) {
        // TODO checking default search count = 500 to avoid failure. Make this
        // configurable
        while (ldapResults.hasMore() && count < 500) {
          try {
            entry = ldapResults.next();
          }
          catch (LDAPException ex) {
            LOGGER.error("Access key find failed.");
            throw new DataAccessException("Access key find failed.\n" + ex);
          }
          String accessKeyId =
              entry.getAttribute(LDAPUtils.ACCESS_KEY_ID).getStringValue();
          accessKey.setId(accessKeyId);
          if (entry.getAttribute(LDAPUtils.EXPIRY) != null) {
            String serverDateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSZ";
            Date expiryDate = DateUtil.toDate(
                entry.getAttribute(LDAPUtils.EXPIRY).getStringValue());
            if (expiryDate.compareTo(new Date()) < 0) {
              delete (accessKey);
              LOGGER.debug("Deleted expired temp key for account - " +
                           user.getAccountName());
            }
          }
          count++;
        }
      }
    }
}



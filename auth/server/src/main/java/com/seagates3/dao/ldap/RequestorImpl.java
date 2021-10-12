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

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.RequestorDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.fi.FaultPoints;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RequestorImpl implements RequestorDAO {

    private final Logger LOGGER =
            LoggerFactory.getLogger(RequestorImpl.class.getName());

    @Override
    public Requestor find(AccessKey accessKey) throws DataAccessException {
        Requestor requestor = new Requestor();

        requestor.setAccessKey(accessKey);

        if (accessKey.getUserId() != null) {
            String filter;
            String[] attrs = {LDAPUtils.COMMON_NAME};
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

            filter = String.format("%s=%s", LDAPUtils.USER_ID,
                    accessKey.getUserId());

            String baseDN = String.format("%s=%s,%s",
                    LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
                    LDAPUtils.BASE_DN);

            LOGGER.debug("Finding access key details of userID: "
                                            + accessKey.getUserId());
            ldapResults = lc.search(baseDN, LDAPConnection.SCOPE_SUB, filter,
                                    attrs, false);
              }
            }
            catch (LDAPException ex) {
              LOGGER.error("Failed to find access key details of userId: " +
                           accessKey.getUserId());
                throw new DataAccessException(
                        "Failed to find requestor details.\n" + ex);
            }
            try {
              if (ldapResults != null && ldapResults.hasMore()) {
                LDAPEntry entry;
                    entry = ldapResults.next();
                requestor.setId(accessKey.getUserId());
                requestor.setName(entry.getAttribute(
                        LDAPUtils.COMMON_NAME).getStringValue());

                String accountName = getAccountName(entry.getDN());
                requestor.setAccount(getAccount(accountName));
                lc.abandon(ldapResults);
            } else {
                LOGGER.error("Failed to find access key details of userId: "
                        + accessKey.getUserId());
                throw new DataAccessException(
                        "Failed to find the requestor who owns the "
                        + "given access key.\n");
            }
            }
            catch (LDAPException ex) {
              throw new DataAccessException("LDAP error\n" + ex);
            }
            finally { LdapConnectionManager.releaseConnection(lc); }
        }
        return requestor;
    }

    /**
     * Extract the account name from user distinguished name.
     *
     * @param dn
     * @return Account Name
     */
    private String getAccountName(String dn) {
        String dnRegexPattern = "[\\w\\W]+,o=(.*?),[\\w\\W]+";

        Pattern pattern = Pattern.compile(dnRegexPattern);
        Matcher matcher = pattern.matcher(dn);
        if (matcher.find()) {
            return matcher.group(1);
        }

        return "";
    }

    /**
     * Get the account details from account name.
     *
     * @param accountName Account name.
     * @return Account
     */
    private Account getAccount(String accountName) throws DataAccessException {
        AccountDAO accountDao = (AccountDAO) DAODispatcher.getResourceDAO(
                DAOResource.ACCOUNT);
        LOGGER.debug("Finding account: " + accountName);
        return accountDao.find(accountName);
    }
}


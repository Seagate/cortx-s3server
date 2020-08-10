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
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.seagates3.dao.AccountLoginProfileDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.util.KeyGenUtil;

import java.util.ArrayList;
import java.security.NoSuchAlgorithmException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
/**
 *
 * TODO : Remove this comment once below code tested end to end
 *
 */
public
class AccountLoginProfileImpl implements AccountLoginProfileDAO {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(UserLoginProfileImpl.class.getName());

 public
  void save(Account account) throws DataAccessException {

    ArrayList<LDAPModification> modList = new ArrayList<LDAPModification>();
    LDAPAttribute attr;
    String password = null;

    String dn =
        String.format("%s=%s,%s=accounts,%s", LDAPUtils.ORGANIZATIONAL_NAME,
                      account.getName(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.BASE_DN);
    try {
      password = KeyGenUtil.generateSSHA(account.getPassword());
    }
    catch (NoSuchAlgorithmException ex) {
      throw new DataAccessException("Failed to modify the account details.\n" +
                                    ex);
    }
    if (password != null) {
      attr = new LDAPAttribute(LDAPUtils.PASSWORD, account.getPassword());
      modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
    }

    LDAPAttribute attrDate = new LDAPAttribute(LDAPUtils.PROFILE_CREATE_DATE,
                                               account.getProfileCreateDate());
    modList.add(new LDAPModification(LDAPModification.REPLACE, attrDate));

    LDAPAttribute attrReset = new LDAPAttribute(
        LDAPUtils.PASSWORD_RESET_REQUIRED, account.getPwdResetRequired());
    modList.add(new LDAPModification(LDAPModification.REPLACE, attrReset));

    try {
      LOGGER.info("Modifying dn " + dn);
      LDAPUtils.modify(dn, modList);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to modify the details of account: " +
                   account.getName());
      throw new DataAccessException("Failed to modify the account" +
                                    " details.\n" + ex);
    }
  }
}

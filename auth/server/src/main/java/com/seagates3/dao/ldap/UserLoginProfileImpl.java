/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author: Abhilekh Mustapure <abhilekh.mustapure@seagate.com>
 * Original creation date: 22-May-2019
 */
package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.UserDAO;
import com.seagates3.dao.UserLoginProfileDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;
import com.seagates3.util.DateUtil;
import java.util.ArrayList;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SignatureException;

import java.security.SecureRandom;
import java.util.Base64;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class UserLoginProfileImpl implements UserLoginProfileDAO {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(UserLoginProfileImpl.class.getName());
  String password = null;
 private
  static final int SALT_LENGTH = 4;

 public
  void save(User user) throws DataAccessException {

    ArrayList modList = new ArrayList();
    LDAPAttribute attr;

    String dn =
        String.format("%s=%s,%s=%s,%s=%s,%s=%s,%s", LDAPUtils.USER_ID,
                      user.getId(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.USER_OU, LDAPUtils.ORGANIZATIONAL_NAME,
                      user.getAccountName(), LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
                      LDAPUtils.ACCOUNT_OU, LDAPUtils.BASE_DN);
    try {
      password = generateSSHA(user.getPassword().getBytes());
    }
    catch (NoSuchAlgorithmException ex) {
      throw new DataAccessException("Failed to modify the user details.\n" +
                                    ex);
    }
    if (password != null) {
      attr = new LDAPAttribute(LDAPUtils.PASSWORD, user.getPassword());
      modList.add(new LDAPModification(LDAPModification.REPLACE, attr));
    }

    LDAPAttribute attrDate = new LDAPAttribute(LDAPUtils.PROFILE_CREATE_DATE,
                                               user.getProfileCreateDate());
    modList.add(new LDAPModification(LDAPModification.REPLACE, attrDate));

    LDAPAttribute attrReset = new LDAPAttribute(
        LDAPUtils.PASSWORD_RESET_REQUIRED, user.getPwdResetRequired());
    modList.add(new LDAPModification(LDAPModification.REPLACE, attrReset));

    try {
      LDAPUtils.modify(dn, modList);
    }
    catch (LDAPException ex) {
      LOGGER.error("Failed to modify the details of user: " + user.getName());
      throw new DataAccessException("Failed to modify the user" +
                                    " details.\n" + ex);
    }
  }

 public
  static String generateSSHA(byte[] password) throws NoSuchAlgorithmException {

    SecureRandom secureRandom = new SecureRandom();
    byte[] salt = new byte[SALT_LENGTH];
    secureRandom.nextBytes(salt);

    MessageDigest crypt = MessageDigest.getInstance("SHA-1");
    crypt.reset();
    crypt.update(password);
    crypt.update(salt);
    byte[] hash = crypt.digest();

    byte[] hashPlusSalt = new byte[hash.length + salt.length];
    System.arraycopy(hash, 0, hashPlusSalt, 0, hash.length);
    System.arraycopy(salt, 0, hashPlusSalt, hash.length, salt.length);

    return new StringBuilder()
        .append("{SSHA}")
        .append(Base64.getEncoder().encodeToString(hashPlusSalt))
        .toString();
  }
}

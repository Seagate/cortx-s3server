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

import java.io.UnsupportedEncodingException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPJSSESecureSocketFactory;
import com.novell.ldap.LDAPSocketFactory;
import com.novell.ldap.connectionpool.PoolManager;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.ServerInitialisationException;
import com.seagates3.fi.FaultPoints;
import com.seagates3.util.IEMUtil;

public class LdapConnectionManager {

    private static final Logger LOGGER = LoggerFactory.getLogger(LdapConnectionManager.class.getName());
    static PoolManager ldapPool;
    static String ldapLoginDN, ldapLoginPW;

    public static void initLdap()
            throws ServerInitialisationException {

        try {
            LDAPSocketFactory socketFactory = null;
            int port;
            if (AuthServerConfig.isSSLToLdapEnabled()) {
                port = AuthServerConfig.getLdapSSLPort();
                LOGGER.info("Connecting ldap on SSL port :" + port);
                socketFactory = new LDAPJSSESecureSocketFactory();
            }
            else {
                port = AuthServerConfig.getLdapPort();
                LOGGER.info("Connecting ldap on port :" + port);
            }

            ldapPool = new PoolManager(AuthServerConfig.getLdapHost(),
                    port,
                    AuthServerConfig.getLdapMaxConnections(),
                    AuthServerConfig.getLdapMaxSharedConnections(),
                    socketFactory);

            ldapLoginDN = AuthServerConfig.getLdapLoginDN();
            ldapLoginPW = AuthServerConfig.getLdapLoginPassword();
        } catch (LDAPException ex) {
            String msg = "Failed to initialise LDAP.\n" + ex.toString();
            if (retryLdapConnection(ex.getResultCode())) {
              LOGGER.error(msg);
              IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.LDAP_EX,
                          "LDAP exception occurred",
                          String.format("\"cause\": \"%s\"", ex.getCause()));
            }
            throw new ServerInitialisationException(msg);
        }
    }

    public static LDAPConnection getConnection() {
        LDAPConnection lc = null;
        try {
            if (FaultPoints.fiEnabled()) {
                if (FaultPoints.getInstance().isFaultPointActive("LDAP_CONNECT_FAIL")) {
                    throw new LDAPException("Connection failed", LDAPException.CONNECT_ERROR,
                            null);
                } else if (FaultPoints.getInstance().isFaultPointActive("LDAP_CONN_INTRPT")) {
                    throw new InterruptedException("Connection interrupted");
                }
            }

            lc = ldapPool.getBoundConnection(
                    ldapLoginDN, ldapLoginPW.getBytes("UTF-8"));
        } catch (LDAPException ex) {
          if (retryLdapConnection(ex.getResultCode())) {
            LOGGER.error("LDAPException Cause: " + ex.getCause() +
                         ". Message: " + ex.getMessage());
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.LDAP_EX, "LDAP exception occurred",
                    String.format("\"cause\": \"%s\"", ex.getCause()));
          }
        } catch (InterruptedException ex) {
            LOGGER.error("Failed to connect to LDAP server. Cause: "
                    + ex.getCause() + ". Message: "
                    + ex.getMessage());
        } catch (UnsupportedEncodingException ex) {
            LOGGER.error("UnsupportedEncodingException Cause: " + ex.getCause()
                       + ". Message: " + ex.getMessage());
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UTF8_UNAVAILABLE,
                    "UTF-8 encoding is not supported", null);
        }
        return lc;
    }

   public
    static boolean retryLdapConnection(int resultcode) {

      // resultcode 91 maps to CONNECT_ERROR,ldap client has either lost
      // connection
      // or can not establish connection to ldap server.
      // resultcode 52 maps to UNAVAILABLE,ldap server can not process client's
      // bind request usually because it is shutting down
      // resultcode 81 maps to ldap libraries cannot establish initial
      // connection with ldap server
      // either ldap server is down or specified hostname or port number is
      // incorrect.
      boolean retryConnectionSuccess = false;
      if (resultcode == 91 || resultcode == 52 || resultcode == 81) {
        // wait on thread to retry new ldap connection

        try {
          Thread.sleep(500);
          LDAPConnection conn = new LDAPConnection(1000);
          conn.connect(AuthServerConfig.getLdapHost(),
                       AuthServerConfig.getLdapPort());
          retryConnectionSuccess = true;
        }
        catch (LDAPException e) {
          LOGGER.error("LDAPException Cause: " + e.getCause() + ". Message: " +
                       e.getMessage());
          IEMUtil.log(IEMUtil.Level.FATAL, IEMUtil.LDAP_EX,
                      "LDAP exception occurred",
                      String.format("\"cause\": \"%s\"", e.getCause()));
        }
        catch (InterruptedException e) {
          LOGGER.error("Reset key  delay failing - ", e);
          Thread.currentThread().interrupt();
        }
      }
      return retryConnectionSuccess;
    }

    public static void releaseConnection(LDAPConnection lc) {
        ldapPool.makeConnectionAvailable(lc);
    }

   public
    static LDAPConnection getConnection(String dn,
                                        String password) throws LDAPException {
      LDAPConnection lc = null;
      try {
        if (FaultPoints.fiEnabled()) {
          if (FaultPoints.getInstance().isFaultPointActive(
                  "LDAP_CONNECT_FAIL")) {
            throw new LDAPException("Connection failed",
                                    LDAPException.CONNECT_ERROR, null);
          } else if (FaultPoints.getInstance().isFaultPointActive(
                         "LDAP_CONN_INTRPT")) {
            throw new InterruptedException("Connection interrupted");
          }
        }

        lc = ldapPool.getBoundConnection(dn, password.getBytes("UTF-8"));
      }
      catch (InterruptedException ex) {
        LOGGER.error("Failed to connect to LDAP server. Cause: " +
                     ex.getCause() + ". Message: " + ex.getMessage());
      }
      catch (UnsupportedEncodingException ex) {
        LOGGER.error("UnsupportedEncodingException Cause: " + ex.getCause() +
                     ". Message: " + ex.getMessage());
        IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UTF8_UNAVAILABLE,
                    "UTF-8 encoding is not supported", null);
      }
      catch (LDAPException ex) {
        if (retryLdapConnection(ex.getResultCode())) {
          LOGGER.error("LDAPException Cause: " + ex.getCause() + ". Message: " +
                       ex.getMessage());
          IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.LDAP_EX,
                      "LDAP exception occurred",
                      String.format("\"cause\": \"%s\"", ex.getCause()));
        }
      }

      return lc;
    }
}


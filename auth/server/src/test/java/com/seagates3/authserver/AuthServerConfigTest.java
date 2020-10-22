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

package com.seagates3.authserver;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.GeneralSecurityException;
import java.util.Properties;

import org.junit.Test;

public class AuthServerConfigTest {

    @Test
    public void initTest() throws Exception {

        Properties authServerConfig = getAuthProperties();
        AuthServerConfig.authResourceDir = "../resources";
        AuthServerConfig.init(authServerConfig);
        assertEquals("127.0.0.1", AuthServerConfig.getDefaultEndpoint());

        assertEquals("resources/static/saml-metadata.xml",
                AuthServerConfig.getSAMLMetadataFilePath());

        assertEquals(9085, AuthServerConfig.getHttpPort());

        assertEquals(9086, AuthServerConfig.getHttpsPort());

        assertEquals("s3authserver.jks_template",
                     AuthServerConfig.getKeyStoreName());

        assertNotNull(AuthServerConfig.getKeyStorePassword());

        assertNotNull(AuthServerConfig.getKeyPassword());

        assertTrue(AuthServerConfig.isHttpsEnabled());

        assertEquals("ldap", AuthServerConfig.getDataSource());

        assertEquals("127.0.0.1", AuthServerConfig.getLdapHost());

        assertEquals(389, AuthServerConfig.getLdapPort());

        assertEquals(636, AuthServerConfig.getLdapSSLPort());

        assertEquals(true, AuthServerConfig.isSSLToLdapEnabled());

        assertEquals(5, AuthServerConfig.getLdapMaxConnections());

        assertEquals(1, AuthServerConfig.getLdapMaxSharedConnections());

        assertEquals("cn=admin,dc=seagate,dc=com", AuthServerConfig.getLdapLoginDN());

        assertEquals("https://console.s3.seagate.com:9292/sso", AuthServerConfig.getConsoleURL());

        assertNull(AuthServerConfig.getLogConfigFile());

        assertNull(AuthServerConfig.getLogLevel());

        assertEquals(1, AuthServerConfig.getBossGroupThreads());

        assertEquals(2, AuthServerConfig.getWorkerGroupThreads());

        assertFalse(AuthServerConfig.isPerfEnabled());

        assertEquals("/var/log/seagate/auth/perf.log", AuthServerConfig.getPerfLogFile());

        assertEquals(4, AuthServerConfig.getEventExecutorThreads());

        assertFalse(AuthServerConfig.isFaultInjectionEnabled());

        String[] expectedEndPoints = {"s3-us-west-2.seagate.com", "s3-us.seagate.com",
                "s3-europe.seagate.com", "s3-asia.seagate.com"};
        assertArrayEquals(expectedEndPoints, AuthServerConfig.getEndpoints());

        Path keyStorePath =
            Paths.get("..", "..", "scripts", "s3authserver.jks_template");
        assertTrue(keyStorePath.toString().equals(
                        AuthServerConfig.getKeyStorePath().toString()));
        assertTrue(AuthServerConfig.isEnableHttpsToS3());
    }

    @Test
    public void loadCredentialsTest() throws GeneralSecurityException, Exception {
        Properties authServerConfig = getAuthProperties();
        AuthServerConfig.authResourceDir = "../resources";
        AuthServerConfig.init(authServerConfig);

        AuthServerConfig.loadCredentials();
        assertEquals("ldapadmin", AuthServerConfig.getLdapLoginPassword());
    }

    @Test
    public void readConfigTest() throws Exception {
      AuthServerConfig.readConfig("../resources",
                                  "authserver.properties.sample",
                                  "keystore.properties.sample");

        assertEquals("127.0.0.1", AuthServerConfig.getDefaultEndpoint());

        assertEquals("resources/static/saml-metadata.xml",
                AuthServerConfig.getSAMLMetadataFilePath());

        assertEquals(9085, AuthServerConfig.getHttpPort());

        assertEquals(9086, AuthServerConfig.getHttpsPort());

        assertEquals("s3authserver.jks", AuthServerConfig.getKeyStoreName());

        assertNotNull(AuthServerConfig.getKeyStorePassword());

        assertNotNull(AuthServerConfig.getKeyPassword());

        assertFalse(AuthServerConfig.isHttpsEnabled());

        assertEquals("ldap", AuthServerConfig.getDataSource());

        assertEquals("127.0.0.1", AuthServerConfig.getLdapHost());

        assertEquals(389, AuthServerConfig.getLdapPort());

        assertEquals(636, AuthServerConfig.getLdapSSLPort());

        assertEquals(false, AuthServerConfig.isSSLToLdapEnabled());
    }

    @Test(expected = IOException.class)
    public void readConfigTest_ShouldThrowIOException() throws Exception {
        //Pass Invalid Path
      AuthServerConfig.readConfig("/invalid/path",
                                  "authserver.properties.sample",
                                  "keystore.properties.sample");
    }

    private Properties getAuthProperties() throws Exception {
        Properties authServerConfig = new Properties();

        authServerConfig.setProperty("s3Endpoints", "s3-us-west-2.seagate.com," +
                "s3-us.seagate.com,s3-europe.seagate.com,s3-asia.seagate.com");
        authServerConfig.setProperty("defaultEndpoint", "127.0.0.1");
        authServerConfig.setProperty("samlMetadataFileName", "saml-metadata.xml");
        authServerConfig.setProperty("nettyBossGroupThreads","1");
        authServerConfig.setProperty("nettyWorkerGroupThreads", "2");
        authServerConfig.setProperty("nettyEventExecutorThreads", "4");
        authServerConfig.setProperty("httpPort", "9085");
        authServerConfig.setProperty("httpsPort", "9086");
        authServerConfig.setProperty("logFilePath", "/var/log/seagate/auth/");
        authServerConfig.setProperty("dataSource", "ldap");
        authServerConfig.setProperty("ldapHost", "127.0.0.1");
        authServerConfig.setProperty("ldapPort", "389");
        authServerConfig.setProperty("ldapSSLPort", "636");
        authServerConfig.setProperty("enableSSLToLdap", "true");
        authServerConfig.setProperty("ldapMaxCons", "5");
        authServerConfig.setProperty("ldapMaxSharedCons", "1");
        authServerConfig.setProperty("ldapLoginDN", "cn=admin,dc=seagate,dc=com");
        authServerConfig.setProperty("ldapLoginPW",
        "Rofaa+mJRYLVbuA2kF+CyVUJBjPx5IIFDBQfajmrn23o5aEZHonQj1ikUU9iMBoC6p/dZtVXMO1KFGzHXX3y1A==");
        authServerConfig.setProperty("consoleURL", "https://console.s3.seagate.com:9292/sso");
        authServerConfig.setProperty("enable_https", "true");
        authServerConfig.setProperty("enable_http", "false");
        authServerConfig.setProperty("enableFaultInjection", "false");
        authServerConfig.setProperty("perfEnabled", "false");
        authServerConfig.setProperty("perfLogFile", "/var/log/seagate/auth/perf.log");
        authServerConfig.setProperty("s3KeyStorePath", "../../scripts/");
        authServerConfig.setProperty("s3KeyStoreName",
                                     "s3authserver.jks_template");
        authServerConfig.setProperty("s3KeyStorePassword", "seagate");
        authServerConfig.setProperty("s3KeyPassword", "seagate");
        authServerConfig.setProperty("s3AuthCertAlias", "s3auth_pass");
        authServerConfig.setProperty("enableHttpsToS3", "true");
        authServerConfig.setProperty("s3CipherUtil",
                                     "cortxsec getkey 123 ldap");

        return authServerConfig;
    }
}


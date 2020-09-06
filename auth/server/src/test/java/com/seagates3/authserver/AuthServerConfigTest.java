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



import org.junit.Test;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.GeneralSecurityException;
import java.util.Properties;

import static org.junit.Assert.*;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.powermock.api.mockito.PowerMockito.doNothing;
import static org.powermock.api.mockito.PowerMockito.doThrow;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.verifyStatic;
import static org.powermock.api.mockito.PowerMockito.whenNew;

public class AuthServerConfigTest {

    @Test
    public void initTest() throws Exception {

        Properties authServerConfig = getAuthProperties();
        AuthServerConfig.authResourceDir = "../resources";
        AuthServerConfig.init(authServerConfig);
        assertEquals("s3.seagate.com", AuthServerConfig.getDefaultEndpoint());

        assertEquals("resources/static/saml-metadata.xml",
                AuthServerConfig.getSAMLMetadataFilePath());

        assertEquals(9085, AuthServerConfig.getHttpPort());

        assertEquals(9086, AuthServerConfig.getHttpsPort());

        assertEquals("s3authserver.jks", AuthServerConfig.getKeyStoreName());

        assertEquals(12, AuthServerConfig.getKeyStorePassword().length());

        assertEquals(12, AuthServerConfig.getKeyPassword().length());

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
             Paths.get("..", "resources", "s3authserver.jks");
        assertTrue(keyStorePath.toString().equals(
                        AuthServerConfig.getKeyStorePath().toString()));
        assertTrue(AuthServerConfig.isEnableHttpsToS3());
    }

    /*
     * @Test public void loadCredentialsTest() throws GeneralSecurityException,
     * Exception { Properties authServerConfig = getAuthProperties();
     * AuthServerConfig.authResourceDir = "../resources";
     * AuthServerConfig.init(authServerConfig);
     * AuthServerConfig.loadCredentials();
     * assertEquals("ldapadmin", AuthServerConfig.getLdapLoginPassword()); }
     */

    @Test
    public void readConfigTest() throws Exception {
        AuthServerConfig.readConfig("../resources");

        assertEquals("s3.seagate.com", AuthServerConfig.getDefaultEndpoint());

        assertEquals("resources/static/saml-metadata.xml",
                AuthServerConfig.getSAMLMetadataFilePath());

        assertEquals(9085, AuthServerConfig.getHttpPort());

        assertEquals(9086, AuthServerConfig.getHttpsPort());

        assertEquals("s3authserver.jks", AuthServerConfig.getKeyStoreName());

        assertEquals(12, AuthServerConfig.getKeyStorePassword().length());

        assertEquals(12, AuthServerConfig.getKeyPassword().length());

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
        AuthServerConfig.readConfig("/invalid/path");
    }

    private Properties getAuthProperties() throws Exception {
        Properties authServerConfig = new Properties();

        authServerConfig.setProperty("s3Endpoints", "s3-us-west-2.seagate.com," +
                "s3-us.seagate.com,s3-europe.seagate.com,s3-asia.seagate.com");
        authServerConfig.setProperty("defaultEndpoint", "s3.seagate.com");
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
        authServerConfig.setProperty("s3KeyStorePath", "../resources/");
        authServerConfig.setProperty("s3KeyStoreName", "s3authserver.jks");
        authServerConfig.setProperty(
            "s3KeyStorePassword",
            "Y2EwZDZmY2Fm");  // random string of length 12
        authServerConfig.setProperty(
            "s3KeyPassword", "Y2EwZDZmY2Fm");  // random string of length 12
        authServerConfig.setProperty("s3AuthCertAlias", "s3auth_pass");
        authServerConfig.setProperty("enableHttpsToS3", "true");

        return authServerConfig;
    }
}

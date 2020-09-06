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

import com.seagates3.exception.ServerInitialisationException;

import io.netty.channel.EventLoopGroup;
import io.netty.handler.ssl.SslContext;
import io.netty.handler.ssl.SslContextBuilder;
import io.netty.util.concurrent.EventExecutorGroup;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLException;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.NoSuchAlgorithmException;
import java.util.Properties;

import static org.junit.Assert.*;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.powermock.api.mockito.PowerMockito.doReturn;
import static org.powermock.api.mockito.PowerMockito.mock;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.when;

@RunWith(PowerMockRunner.class)
@PrepareForTest({AuthServerConfig.class, KeyManagerFactory.class, SslContextBuilder.class})
@MockPolicy(Slf4jMockPolicy.class)
@PowerMockIgnore({"javax.management.*"})
public class SSLContextProviderTest {

    private static Path filePath = Paths.get("..", "resources",
          "s3authserver.jks");
    private
     static Path KeyStorePropertiesFilePath =
         Paths.get("..", "resources", "keystore.properties");
    private
     static String keyStorePasswd = "";
    private
     static String keyPasswd = "";

    @Before
    public void setUp() throws Exception {
        mockStatic(AuthServerConfig.class);
        when(AuthServerConfig.getHttpsPort()).thenReturn(9086);
        when(AuthServerConfig.isHttpsEnabled()).thenReturn(Boolean.TRUE);
        fetch_keystore_password_from_properties_file();
    }

    /*
     * Passwords are generated randomly hence need to fetch it before running
     * tests
     */
   private
    void fetch_keystore_password_from_properties_file() throws IOException {
      Properties keystoreprop = new Properties();
      InputStream in =
          new FileInputStream(KeyStorePropertiesFilePath.toString());
      keystoreprop.load(in);
      keyStorePasswd = (String)keystoreprop.get("s3KeyStorePassword");
      keyPasswd = (String)keystoreprop.get("s3KeyPassword");
      if (in != null) {
        in.close();
      }
    }

    @Test public void initTest_HttpsDisabled()
        throws ServerInitialisationException {
        when(AuthServerConfig.isHttpsEnabled()).thenReturn(Boolean.FALSE);

        when(AuthServerConfig.getKeyStorePath()).thenReturn(Paths.get("..",
            "resources", "s3authserver.jks"));
        SSLContextProvider.init();

        assertNull(SSLContextProvider.getServerContext());
    }

    @Test
    public void initTest_HttpsEnabled() throws ServerInitialisationException,
            NoSuchAlgorithmException, SSLException, Exception{
        mockStatic(KeyManagerFactory.class);
        mockStatic(SslContextBuilder.class);

        KeyManagerFactory kmf = mock(KeyManagerFactory.class);
        SslContextBuilder contextBuilder = mock(SslContextBuilder.class);
        SslContext sslContext = mock(SslContext.class);
        when(AuthServerConfig.getKeyStorePath()).thenReturn(Paths.get("..",
                "resources", "s3authserver.jks"));
        when(AuthServerConfig.getKeyStorePassword()).thenReturn(keyStorePasswd);
        when(AuthServerConfig.getKeyPassword()).thenReturn(keyPasswd);
        when(KeyManagerFactory.getInstance(anyString())).thenReturn(kmf);
        when(SslContextBuilder.forServer(kmf)).thenReturn(contextBuilder);

        when(contextBuilder.build()).thenReturn(sslContext);

        SSLContextProvider.init();

        assertEquals(sslContext, SSLContextProvider.getServerContext());
        assertEquals(9086, SSLContextProvider.getHttpsPort());
    }

    @Test(expected = ServerInitialisationException.class)
    public void initTest_HttpsEnabled_NoSuchAlgorithm()
                                    throws ServerInitialisationException {
        when(AuthServerConfig.getKeyStorePath()).thenReturn(Paths.get("..",
                "resources", "s3authserver.jks"));
        when(AuthServerConfig.getKeyStorePassword()).thenReturn(keyStorePasswd);
        when(AuthServerConfig.getKeyPassword()).thenReturn(keyPasswd);

        SSLContextProvider.init();
    }
}

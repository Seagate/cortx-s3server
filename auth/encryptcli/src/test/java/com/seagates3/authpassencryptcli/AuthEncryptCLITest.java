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

package com.seagates3.authpassencryptcli;


import java.io.FileInputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.PublicKey;
import java.util.Properties;

import org.apache.logging.log4j.core.config.Configurator;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;

import com.seagates3.authencryptutil.AuthEncryptConfig;
import com.seagates3.authencryptutil.RSAEncryptDecryptUtil;

@RunWith(PowerMockRunner.class)
@MockPolicy(Slf4jMockPolicy.class)
@PowerMockIgnore({"javax.management.*", "javax.crypto.*"})


public  class AuthEncryptCLITest {

    @Before
    public void setUp() throws Exception {
        //For Unit Tests resource folder is at s3server/auth/resources
        String installDir = "..";
        Logger logger = mock(Logger.class);
        AuthEncryptConfig.readConfig(installDir +
                                     "/resources/keystore.properties.sample");
        AuthEncryptConfig.overrideProperty("s3KeyStorePath", "../resources");
        WhiteboxImpl.setInternalState(AuthEncryptCLI.class, "logger", logger);
    }

    @Test
    public void testProcessEncryptRequestNegative() throws Exception {
        String passwd = "";
        String encryptedPasswd;
        try {
            encryptedPasswd = AuthEncryptCLI.processEncryptRequest(passwd);
            fail("Expecting a exception when password is empty");
        } catch (Exception e) {
            assertEquals(e.getMessage(), "Invalid Password Value.");
        }

        passwd = null;
        try {
            encryptedPasswd = AuthEncryptCLI.processEncryptRequest(passwd);
            fail("Expecting a exception when password is null");
        } catch (Exception e) {
            assertEquals(e.getMessage(), "Invalid Password Value.");
        }

        passwd = "test space";
        try {
            encryptedPasswd = AuthEncryptCLI.processEncryptRequest(passwd);
            fail("Expecting a exception when password contains spaces");
        } catch (Exception e) {
            assertEquals(e.getMessage(), "Invalid Password Value.");
        }
    }
}

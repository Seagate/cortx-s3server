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



package com.seagates3.authencryptutil;

import org.junit.Test;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Properties;

import static org.junit.Assert.*;

public class AuthEncryptConfigTest {

    @Test
    public void initTest() throws Exception {
        Properties authEncryptConfig = getAuthProperties();
        AuthEncryptConfig.init(authEncryptConfig);

        assertEquals("s3authserver.jks", AuthEncryptConfig.getKeyStoreName());

        assertEquals("seagate", AuthEncryptConfig.getKeyStorePassword());

        assertEquals("seagate", AuthEncryptConfig.getKeyPassword());
        assertEquals("passencrypt", AuthEncryptConfig.getCertAlias());
        assertEquals("DEBUG", AuthEncryptConfig.getLogLevel());
    }

    @Test
    public void testReadConfig() throws Exception {
        String installDir = "..";
        AuthEncryptConfig.readConfig(installDir);

        assertEquals("s3authserver.jks", AuthEncryptConfig.getKeyStoreName());

        assertEquals("seagate", AuthEncryptConfig.getKeyStorePassword());

        assertEquals("seagate", AuthEncryptConfig.getKeyPassword());
        assertEquals("s3auth_pass", AuthEncryptConfig.getCertAlias());

        installDir = "invaliddir";
        try {
            AuthEncryptConfig.readConfig(installDir);
        } catch (FileNotFoundException e) {
            String errMsg = e.getMessage();
            assertTrue(e.getMessage().equals("invaliddir/resources/keystore.properties (No such file or directory)"));
        }
    }

    private Properties getAuthProperties() throws Exception {
        Properties AuthEncryptConfig = new Properties();

        AuthEncryptConfig.setProperty("s3KeyStoreName", "s3authserver.jks");
        AuthEncryptConfig.setProperty("s3KeyStorePassword", "seagate");
        AuthEncryptConfig.setProperty("s3KeyPassword", "seagate");
        AuthEncryptConfig.setProperty("s3AuthCertAlias", "passencrypt");
        AuthEncryptConfig.setProperty("logLevel", "DEBUG");
        return AuthEncryptConfig;
    }
}

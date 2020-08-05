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

import static org.junit.Assert.*;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertEquals;
import java.security.GeneralSecurityException;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.PrivateKey;
import java.security.PublicKey;
import org.junit.Before;
import org.junit.Test;

import com.seagates3.authencryptutil.RSAEncryptDecryptUtil;

public class RSAEncryptDecryptUtilTest {
    private PrivateKey privateKey;
    private PublicKey publicKey;
    private String sampleText = "Pass@w0rd!430";
    String encryptedText;

    @Before
    public void setUp() throws Exception {
        KeyPairGenerator keyPairGenerator = KeyPairGenerator.getInstance("RSA");
        keyPairGenerator.initialize(2048);
        KeyPair keyPair = keyPairGenerator.generateKeyPair();
        privateKey = keyPair.getPrivate();
        publicKey = keyPair.getPublic();
    }

    @Test
    public void testEncryptDecrypt() {
        try {
            encryptedText = RSAEncryptDecryptUtil.encrypt(sampleText, publicKey);
            assertNotNull(encryptedText);
            assertFalse(sampleText.equals(encryptedText));
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        }
        try {
            String plainText = RSAEncryptDecryptUtil.decrypt(encryptedText, privateKey);
            assertNotNull(plainText);
            assertTrue(plainText.equals(sampleText));
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        }
    }

    @Test
    public void testNegativeEncryptDecrypt() {
        try {
            String nullText = null;
            encryptedText = RSAEncryptDecryptUtil.encrypt(nullText, publicKey);
            fail("Expecting a excption to be thrown");
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
             assertEquals(e.getMessage(), "Input text is null or empty");
        }

        try {
            String nullText = "";
            encryptedText = RSAEncryptDecryptUtil.encrypt(nullText, publicKey);
            fail("Expecting a excption to be thrown");
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
             assertEquals(e.getMessage(), "Input text is null or empty");
        }

        try {
        	encryptedText = null;
            sampleText = RSAEncryptDecryptUtil.decrypt(encryptedText, privateKey);
            fail("Expecting a excption to be thrown");
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
             assertEquals(e.getMessage(), "Encrypted Text is null or empty");
        }

        try {
        	encryptedText = "";
            sampleText = RSAEncryptDecryptUtil.decrypt(encryptedText, privateKey);
            fail("Expecting a excption to be thrown");
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
             assertEquals(e.getMessage(), "Encrypted Text is null or empty");
        }

        try {
            String inText = "test";
            PublicKey pKey = null;
            encryptedText = RSAEncryptDecryptUtil.encrypt(inText, pKey);
            fail("Expecting a excption to be thrown");
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
             assertEquals(e.getMessage(), "Invalid Public Key");
        }

        try {
        	encryptedText = "hfuoerhfow8028r200hfh20";
        	PrivateKey pvtKey = null;
            sampleText = RSAEncryptDecryptUtil.decrypt(encryptedText, pvtKey);
            fail("Expecting a excption to be thrown");
        } catch (GeneralSecurityException e) {
            fail("Should not have thrown excption, Error Stack:" + e.getStackTrace());
        } catch (Exception e) {
             assertEquals(e.getMessage(), "Invalid Private Key");
        }
    }

}

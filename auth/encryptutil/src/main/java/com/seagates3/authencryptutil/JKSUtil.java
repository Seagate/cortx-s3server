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


import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import java.security.cert.Certificate;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.security.GeneralSecurityException;
import java.security.Key;
import java.security.KeyStore;
import java.security.PrivateKey;
import java.security.PublicKey;

/**
 * The Abstract class to retrieves Public key and Private key corresponding
 * to alias from Java Key store file.
 */
public class JKSUtil {
    /**
     * This method retrieves Public key corresponding to given alias from
     * Java Key store file.
     * @param jksFilePath : Location of java Key Store Path
     * @param alias : Certificate Alias
     * @param password : Java Key Store Password
     * @return PublicKey
     * @throws GeneralSecurityException
     */
    private static final Logger LOGGER = LoggerFactory.getLogger(
            JKSUtil.class.getName());
    public static PublicKey getPublicKeyFromJKS(String jksFilePath,
                            String alias, String password)
                            throws GeneralSecurityException {
        KeyStore keyStore = KeyStore.getInstance("JKS");
        char[] keystorePassword = password.toCharArray();
        try {
            FileInputStream fin = new FileInputStream(jksFilePath);
            keyStore.load(fin, keystorePassword);
            Certificate cert = keyStore.getCertificate(alias);
            if(cert == null) {
            	LOGGER.error("Failed to find get keypair from java key store");
            	return null;
            }
            PublicKey publicKey = cert.getPublicKey();
            return publicKey;
        } catch (FileNotFoundException e) {
            LOGGER.error("Failed to open Java Key store file. Cause:" +
                                   e.getCause() + ". Message:" +
                                   e.getMessage());
        } catch (IOException e) {
            LOGGER.error("Failed to Load Java Key store file. Cause:"  +
                    e.getCause() + ". Message:" +
                    e.getMessage());
        }
        LOGGER.error("Failed to find public key from Java Key Store File");
        return null;
    }

    /**
     * This method retrieves Private key corresponding to alias from Java Key store file.
     * @param jksFilePath : Location of java Key Store Path
     * @param alias : Certificate Alias
     * @param password : Java Key Store Password
     * @return PrivateKey
     * @throws GeneralSecurityException
     */
    public static PrivateKey getPrivateKeyFromJKS(String jksFilePath,
                             String alias, String password)
                             throws GeneralSecurityException {
        KeyStore keyStore = KeyStore.getInstance("JKS");
        char[] keystorePassword = password.toCharArray();
        FileInputStream fin;
        try {
            fin = new FileInputStream(jksFilePath);
            keyStore.load(fin, keystorePassword);
            Key key = keyStore.getKey(alias, keystorePassword);
            if(key instanceof PrivateKey) {
                return (PrivateKey)key;
            }
        } catch (FileNotFoundException e) {
        	LOGGER.error("Failed to open Java Key store file. Cause:" +
                    e.getCause() + ". Message:" +
                    e.getMessage());
        } catch (IOException e) {
        	LOGGER.error("Failed to open Java Key store file. Cause:" +
                    e.getCause() + ". Message:" +
                    e.getMessage());

        }
        LOGGER.error("Failed to find private key from Java Key Store File");
        return null;
    }
}

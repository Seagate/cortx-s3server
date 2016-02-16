/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 23-Dec-2015
 */
package com.seagates3.authserver;

import io.netty.handler.ssl.SslContext;
import io.netty.handler.ssl.SslContextBuilder;
import java.io.FileInputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.KeyStore;
import javax.net.ssl.KeyManagerFactory;

public class SSLContextProvider {

    private static int httpsPort;
    private static SslContext authSSLContext;

    public static void init() throws Exception {
        if (AuthServerConfig.isHttpsEnabled()) {
            httpsPort = AuthServerConfig.getHttpsPort();

            String s3KeystoreName = AuthServerConfig.getKeyStoreName();
            Path s3KeystoreFile = Paths.get("", "resources", s3KeystoreName);

            char[] keystorePassword = AuthServerConfig.getKeyStorePassword()
                    .toCharArray();
            char[] keyPassword = AuthServerConfig.getKeyPassword().toCharArray();

            FileInputStream fin = new FileInputStream(s3KeystoreFile.toFile());
            KeyStore s3Keystore = KeyStore.getInstance("JKS");
            s3Keystore.load(fin, keystorePassword);

            KeyManagerFactory kmf = KeyManagerFactory.getInstance(
                    KeyManagerFactory.getDefaultAlgorithm());
            kmf.init(s3Keystore, keyPassword);

            authSSLContext = SslContextBuilder.forServer(kmf).build();
        } else {
            authSSLContext = null;
        }
    }

    public static SslContext getServerContext() {
        return authSSLContext;
    }

    public static int getHttpsPort() {
        return httpsPort;
    }
}

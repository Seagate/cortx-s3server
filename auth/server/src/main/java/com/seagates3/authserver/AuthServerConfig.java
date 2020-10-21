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

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.management.ManagementFactory;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.GeneralSecurityException;
import java.security.PrivateKey;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.MDC;
import java.util.List;
import com.seagates3.authencryptutil.JKSUtil;
import com.seagates3.authencryptutil.RSAEncryptDecryptUtil;


/**
 * Store the auth server configuration properties like default endpoint and
 * server end points.
 */
public class AuthServerConfig {

    public static String authResourceDir;
    private static String samlMetadataFilePath;
    private static Properties authServerConfig;
    private static String ldapPasswd;
    public
     static final String DEFAULT_ACL_XML = "/defaultAclTemplate.xml";
    public
     static final String XSD_PATH = "/AmazonS3.xsd";
    public
     static final int MAX_GRANT_SIZE = 100;
    private static Logger logger;

    /**
     * Read the properties file.
     * @throws GeneralSecurityException
     */
    public
     static void readConfig(String resourceDir, String authServerFile,
                            String keyStoreFile) throws FileNotFoundException,
         IOException, GeneralSecurityException, Exception {
        authResourceDir = resourceDir;
        Path authProperties = Paths.get(authResourceDir, authServerFile);
        Path authSecureProperties = Paths.get(authResourceDir, keyStoreFile);
        Properties authServerConfig = new Properties();
        InputStream input = new FileInputStream(authProperties.toString());
        authServerConfig.load(input);
        Properties authSecureConfig = new Properties();
        InputStream inSecure = new FileInputStream(authSecureProperties.toString());
        authSecureConfig.load(inSecure);
        authServerConfig.putAll(authSecureConfig);
        AuthServerConfig.init(authServerConfig);
        if (input != null) input.close();
        if (inSecure != null) inSecure.close();
    }

    /**
     * Initialize default endpoint and s3 endpoints etc.
     *
     * @param authServerConfig Server configuration parameters.
     * @throws Exception
     */
    public static void init(Properties authServerConfig) throws Exception {
        AuthServerConfig.authServerConfig = authServerConfig;

        setSamlMetadataFile(authServerConfig.getProperty(
                "samlMetadataFileName"));
        String jvm = ManagementFactory.getRuntimeMXBean().getName();
        AuthServerConfig.authServerConfig.put("pid", jvm.substring(0, jvm.indexOf("@")));

    }

    /**
     * Log default configurations for auth-server
     *
     * @param authServerConfig Server configuration parameters.
     * @param enuKeys ServerConfig keys.
     */
   public static void logConfigProps() {
       Properties authServerConfig = AuthServerConfig.authServerConfig;
       Enumeration<Object> authProps = authServerConfig.keys();

       logger = LoggerFactory.getLogger(AuthServerConfig.class.getName());

       logger.info("Configuring AuthServer with following properties");
       while (authProps.hasMoreElements()) {
                String key = (String) authProps.nextElement();
                Set<String> secureProps = new HashSet<>(Arrays.asList("s3KeyPassword",
                       "ldapLoginPW", "s3KeyStorePassword"));
                if( !secureProps.contains(key) ) {
                     String value = authServerConfig.getProperty(key);
                     logger.info("Config [" + key + "] = " + value);
                }
       }
    }

    /**
     * Initialize Ldap-Password
     *
     * @param authServerConfig Server configuration parameters.
     * @throws GeneralSecurityException
     */
    public static void loadCredentials() throws GeneralSecurityException, Exception {
       logger = LoggerFactory.getLogger(AuthServerConfig.class.getName());
       logger.debug("Loading Openldap credentials");
       String keystorePasswd = null;
       String encryptedPasswd = authServerConfig.getProperty("ldapLoginPW");
       Path keyStoreFilePath = getKeyStorePath();
       try {
         String cmd = authServerConfig.getProperty("s3CipherUtil");
         Process s3Cipher = Runtime.getRuntime().exec(cmd);
         int exitVal = s3Cipher.waitFor();
         if (exitVal != 0) {
           logger.debug("S3 Cipher util failed to return keystore password");
           throw new IOException("S3 cipher util exited with error.");
         }
         BufferedReader reader = new BufferedReader(
             new InputStreamReader(s3Cipher.getInputStream()));
         String line = reader.readLine();
         if (line == null || line.isEmpty()) {
           throw new IOException("S3 cipher returned empty stream.");
         } else {
           keystorePasswd = line;
         }
       }
       catch (Exception e) {
         logger.debug(
             e.getMessage() +
             " IO error in S3 cipher. Loading default keystore credentilas.");
         keystorePasswd = getKeyStorePassword();
       }
       PrivateKey privateKey = JKSUtil.getPrivateKeyFromJKS(
           keyStoreFilePath.toString(), getCertAlias(), keystorePasswd);
       if (privateKey == null) {
         throw new GeneralSecurityException("Failed to find Private Key [" +
                                            keyStoreFilePath + "].");
       }
       ldapPasswd = RSAEncryptDecryptUtil.decrypt(encryptedPasswd, privateKey);
    }

    /**
     * @return the process id
     */
    public static int getPid() {
        return Integer.parseInt(authServerConfig.getProperty("pid"));
    }

    /**
     * Return the end points of S3-Auth server.
     *
     * @return server endpoints.
     */
    public static String[] getEndpoints() {
        return authServerConfig.getProperty("s3Endpoints").split(",");
    }

    /**
     * Return the default end point of S3-Auth server.
     *
     * @return default endpoint.
     */
    public static String getDefaultEndpoint() {
        return authServerConfig.getProperty("defaultEndpoint");
    }

    /**
     * @return Path of SAML metadata file.
     */
    public static String getSAMLMetadataFilePath() {
        return samlMetadataFilePath;
    }

    public static int getHttpPort() {
        return Integer.parseInt(authServerConfig.getProperty("httpPort"));
    }

    public static int getHttpsPort() {
        return Integer.parseInt(authServerConfig.getProperty("httpsPort"));
    }

    public static String getDefaultHost() {
        return authServerConfig.getProperty("defaultHost");
    }

    public static String getKeyStoreName() {
        return authServerConfig.getProperty("s3KeyStoreName");
    }

    public static Path getKeyStorePath() {
        return Paths.get(authServerConfig.getProperty("s3KeyStorePath"),
                         getKeyStoreName());
    }

    public static String getKeyStorePassword() {
        return authServerConfig.getProperty("s3KeyStorePassword");
    }

    public static String getKeyPassword() {
        return authServerConfig.getProperty("s3KeyPassword");
    }

    public static boolean isHttpEnabled() {
        return Boolean.valueOf(authServerConfig.getProperty("enable_http"));
    }

    public static boolean isHttpsEnabled() {
        return Boolean.valueOf(authServerConfig.getProperty("enable_https"));
    }

    public static String getDataSource() {
        return authServerConfig.getProperty("dataSource");
    }

    public static String getLdapHost() {
        return authServerConfig.getProperty("ldapHost");
    }

    public static int getLdapPort() {
        return Integer.parseInt(authServerConfig.getProperty("ldapPort"));
    }

    public static int getLdapSSLPort() {
        return Integer.parseInt(authServerConfig.getProperty("ldapSSLPort"));
    }

    public static Boolean isSSLToLdapEnabled() {
        return Boolean.valueOf(authServerConfig.getProperty("enableSSLToLdap"));
    }

    public static int getLdapMaxConnections() {
        return Integer.parseInt(authServerConfig.getProperty("ldapMaxCons"));
    }

    public static int getLdapMaxSharedConnections() {
        return Integer.parseInt(authServerConfig.getProperty("ldapMaxSharedCons"));
    }

    public static String getLdapLoginDN() {
        return authServerConfig.getProperty("ldapLoginDN");
    }

    public static String getLdapLoginCN() {
        String ldapLoginDN = authServerConfig.getProperty("ldapLoginDN");
        String ldapLoginCN = ldapLoginDN.substring(
              ldapLoginDN.indexOf("cn=") + 3, ldapLoginDN.indexOf(','));
        return ldapLoginCN;
    }

    public static String getLdapLoginPassword() {
        return ldapPasswd;
    }

    public static String getCertAlias() {
        return authServerConfig.getProperty("s3AuthCertAlias");
    }

    public static String getConsoleURL() {
        return authServerConfig.getProperty("consoleURL");
    }

    public static String getLogConfigFile() {
        return authServerConfig.getProperty("logConfigFile");
    }

    public static String getLogLevel() {
        return authServerConfig.getProperty("logLevel");
    }

    public static int getBossGroupThreads() {
        return Integer.parseInt(
                authServerConfig.getProperty("nettyBossGroupThreads"));
    }

    public static int getWorkerGroupThreads() {
        return Integer.parseInt(
                authServerConfig.getProperty("nettyWorkerGroupThreads"));
    }

    public static boolean isPerfEnabled() {
        return Boolean.valueOf(authServerConfig.getProperty("perfEnabled"));
    }

    public static String getPerfLogFile() {
        return authServerConfig.getProperty("perfLogFile");
    }

    public static int getEventExecutorThreads() {
        return Integer.parseInt(
                authServerConfig.getProperty("nettyEventExecutorThreads"));
    }

    public static boolean isFaultInjectionEnabled() {
        return Boolean.valueOf(authServerConfig.getProperty("enableFaultInjection"));
    }

    /**
     * Set the SAML Metadata file Path.
     *
     * @param fileName Name of the metadata file.
     */
    private static void setSamlMetadataFile(String fileName) {
        Path filePath = Paths.get("", "resources", "static", fileName);
        samlMetadataFilePath = filePath.toString();
    }

    public static boolean isEnableHttpsToS3() {
       return Boolean.valueOf(authServerConfig.getProperty("enableHttpsToS3"));
    }

    /**
     * Set Request ID
     */
   public
    static void setReqId(String reqId) { MDC.put("ReqId", reqId); }
    /**
     * Get Request ID
     */
   public
    static String getReqId() {
      String reqId = MDC.get("ReqId");
      if (reqId == null) {
        // Set to some dummy value for Unit tests where Req Id is not set
        reqId = "0000";
      }
      return reqId;
    }
   public
    static List<String> getS3InternalAccounts() {
      List<String> internalAccountsList = new ArrayList<>();
      String internalAccounts =
          authServerConfig.getProperty("s3InternalAccounts");
      if (internalAccounts != null) {
        internalAccountsList = Arrays.asList(internalAccounts.split(","));
      }
      return internalAccountsList;
    }

   public
    int getVersion() {
      return Integer.parseInt(authServerConfig.getProperty("version"));
    }
}


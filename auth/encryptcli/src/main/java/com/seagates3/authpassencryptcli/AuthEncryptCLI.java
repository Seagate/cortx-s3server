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

import java.io.BufferedReader;
import java.io.Console;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.GeneralSecurityException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.util.Arrays;
import java.util.Properties;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.config.ConfigurationSource;
import org.apache.logging.log4j.core.config.Configurator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.authencryptutil.AuthEncryptConfig;
import com.seagates3.authencryptutil.JKSUtil;
import com.seagates3.authencryptutil.RSAEncryptDecryptUtil;
import com.seagates3.exception.CLIInitializationException;


/**
 * AuthEncryptCLI class
 *
 */
public class AuthEncryptCLI {

  public
   static final String AUTH_INSTALL_DIR = "/opt/seagate/cortx/auth";
  public
   static final String AUTH_RESOURCE_DIR = "/opt/seagate/cortx/auth/resources";
    private static Logger logger;
    private
     static boolean isEncryptOptionPassed = false;
    private
     static boolean isDecryptOptionPassed = false;

    /**
     * @param args
     */
    public static void usage() {
        System.out.println("Usage: java -jar AuthPassEncryptCLI-1.0-0.jar"
                                                          + " [options]");
        System.out.println(
            "-s  <password>       Specify password to be encrypted");
        System.out.println("-d   decrypt ldap login password");
        System.out.println("-help");
    }

    /**
     * This method reads password from console
     * @return String
     */
    public static String readFromConsole() {
        Console console = System.console();
        if (console == null) {
            System.err.println("Couldn't get Console instance.");
            System.exit(1);
        }
        char[] tmp1 = console.readPassword("Enter Password:");
        char[] tmp2 = console.readPassword("Re-enter Password:");
        if(!Arrays.equals(tmp1, tmp2)) {
            System.err.println("Password values do not match.");
            System.exit(1);
        }
        Arrays.fill(tmp1, ' ');

        return new String(tmp2);
    }

    /**
     * This method parses command line arguments and return password
     * @param args
     * @return String
     */
    public static String parseArgs(String[] args) {

        String passwd = null;
        if(args.length > 0) {
          if (args[0].equals("-help")) {
                usage();
                System.exit(0);
            } else if((args[0].equals("-s")) && (args.length == 2)) {
                passwd = args[1];
                isEncryptOptionPassed = true;
            } else if (args[0].equals("-d")) {
              isDecryptOptionPassed = true;
            } else {
                logger.error("Incorrect arguments passed.");
                usage();
                System.exit(1);
            }
        }
        return passwd;
    }

    /**
     * This method reads keystore password from s3cipher utility.
     */
   public
    static String getKeystorePasswd() throws Exception {
        String keystorePasswd = null;
        try {
          String cmd = AuthEncryptConfig.getCipherUtil();
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
              "{} IO error in S3 cipher. Loading default keystore credentilas.",
              e.getMessage());
          keystorePasswd = AuthEncryptConfig.getKeyStorePassword();
        }
        return keystorePasswd;
    }

    /**
     * This method decrypts password using public key from key store
     * @param passwd to be decrypted
     * @throws GenralSequrityExceptoion
     */
   public
    static String processDecryptRequest(String passwd)
        throws GeneralSecurityException,
        Exception {
      if (passwd == null || passwd.isEmpty() || passwd.matches(".*([ \t]).*")) {
        logger.error("Password is null or empty or contains spaces");
        System.err.println("Invalid password value.");
        throw new Exception("Invalid password value.");
      }
      Path s3KeystoreFile = AuthEncryptConfig.getKeyStorePath();
        String certAlias = AuthEncryptConfig.getCertAlias();
        String keystorePasswd = getKeystorePasswd();

        PrivateKey privateKey = JKSUtil.getPrivateKeyFromJKS(
            s3KeystoreFile.toString(), certAlias, keystorePasswd);

        String decryptedPasswd =
            RSAEncryptDecryptUtil.decrypt(passwd, privateKey);
        if (null == decryptedPasswd || decryptedPasswd.isEmpty()) {
          logger.error("Encrypted Password is null or empty");
          throw new Exception("Failed to decrypt password.");
        }
        return decryptedPasswd;
    }

    /**
     * This method encrypts password using public key from key store
     * @param passwd
     * @throws GeneralSecurityException
     * @throws Exception
     */
   public
    static String processEncryptRequest(String passwd)
        throws GeneralSecurityException,
        Exception {

      String encryptedPasswd = null;
      if (passwd == null || passwd.isEmpty() || passwd.matches(".*([ \t]).*")) {
        logger.error("Password is null or empty or contains spaces");
        System.err.println("Invalid Password Value.");
        throw new Exception("Invalid Password Value.");
      }
      Path s3KeystoreFile = AuthEncryptConfig.getKeyStorePath();
      logger.debug("Using Java Key Store File: {}", s3KeystoreFile);
      String certAlias = AuthEncryptConfig.getCertAlias();
      String keystorePasswd = getKeystorePasswd();

      PublicKey pKey = JKSUtil.getPublicKeyFromJKS(s3KeystoreFile.toString(),
                                                   certAlias, keystorePasswd);
      logger.debug("Public Key: {}", pKey);
        encryptedPasswd = RSAEncryptDecryptUtil.encrypt(passwd, pKey);

        if(encryptedPasswd == null || encryptedPasswd.isEmpty()) {
            logger.error("Encrypted Password is null or empty");
            throw new Exception("Failed to encrypt password.");
        }
        return encryptedPasswd;
    }

    /**
     * This method initializes logger
     * @throws IOException
     */
    static void logInit() throws CLIInitializationException, Exception {

        AuthEncryptConfig.setLogConfigFile();
        String logConfigFilePath = AuthEncryptConfig.getLogConfigFile();

        /**
         * If log4j config file is given, override the default Logging
         * properties file.
         */
        if (logConfigFilePath != null) {
            File logConfigFile = new File(logConfigFilePath);
            if (logConfigFile.exists()) {
                ConfigurationSource source = new ConfigurationSource(
                        new FileInputStream(logConfigFile));
                Configurator.initialize(null, source);
            } else {
                throw new CLIInitializationException("Logging config file "
                                  + logConfigFilePath + " doesn't exist.");
            }
        }

        String logLevel = AuthEncryptConfig.getLogLevel();
        if (logLevel != null) {
            Level level = Level.getLevel(logLevel);
            if (level == null) {
                //set to ALL if couldn't find appropriate level value.
                throw new Exception("Invalid log value [" + logLevel + "]"
                                            + "specified in config file");
            } else {
                Configurator.setRootLevel(level);
            }
        }
    }

    public static void main(String[] args) {

        try {
          AuthEncryptConfig.readConfig(AUTH_INSTALL_DIR +
                                       "/resources/keystore.properties");
        } catch (IOException e1) {
          logger.error(
              "Failed to read Properties from install dir: {}. Error Stack: {}",
              AUTH_INSTALL_DIR, e1.getStackTrace());
            System.err.println("Failed to read Properties from install dir:"
                                                        + AUTH_INSTALL_DIR);
            System.exit(1);
        }
        try {
            logInit();
            logger = LoggerFactory.getLogger(AuthEncryptCLI.class.getName());
        } catch (CLIInitializationException e) {
            System.err.println("Failed to Initialize logger");
            e.printStackTrace();
            System.exit(1);
        } catch (Exception e) {

        }
        String passwd = null;

        logger.debug("Initialization successfull");
        if(args.length == 0) {
            passwd = readFromConsole();
        } else {
            passwd = parseArgs(args);
        }
        try {
          if (isEncryptOptionPassed) {
            // Output to console
            System.out.println(processEncryptRequest(passwd));
          } else if (isDecryptOptionPassed) {
            Properties authServerConfig = new Properties();
            Path authProperties =
                Paths.get(AUTH_RESOURCE_DIR, "authserver.properties");
            InputStream inStream =
                new FileInputStream(authProperties.toString());
            authServerConfig.load(inStream);

            String encryptedPasswd =
                authServerConfig.getProperty("ldapLoginPW");

            // Output to console
            System.out.println(processDecryptRequest(encryptedPasswd));
            inStream.close();
          }
        } catch (GeneralSecurityException e) {
          logger.error("Failed to encrypt/decrypt password. Cause: {}, msg: {}",
                       e.getCause(), e.getMessage());
          System.err.println("Failed to encrypt/decrypt password.");
            System.exit(1);
        } catch (Exception e) {
          logger.error("Failed to encrypt/decrypt password. Cause: {}, msg: {}",
                       e.getCause(), e.getMessage());
            System.err.println("Failed to encrypt password.");
            System.exit(1);
        }
        logger.debug("Operation completed successfully");
    }
}

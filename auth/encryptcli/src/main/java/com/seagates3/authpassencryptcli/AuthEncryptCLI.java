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
import java.io.InputStreamReader;

import java.nio.file.Path;
import java.security.GeneralSecurityException;
import java.security.PublicKey;
import java.util.Arrays;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.config.ConfigurationSource;
import org.apache.logging.log4j.core.config.Configurator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.authencryptutil.AESEncryptDecryptUtil;
import com.seagates3.authencryptutil.AuthEncryptConfig;
import com.seagates3.authencryptutil.EncryptionAlgorithm;
import com.seagates3.authencryptutil.JKSUtil;
import com.seagates3.authencryptutil.RSAEncryptDecryptUtil;
import com.seagates3.exception.CLIInitializationException;

/**
 * AuthEncryptCLI class
 *
 */
public class AuthEncryptCLI {

  public
   static String AUTH_INSTALL_DIR = "/opt/seagate/cortx/auth";
  private
   static Logger logger;

   /**
    * @param args
    */
  public
   static void usage() {
     System.out.println("Usage: java -jar AuthPassEncryptCLI-1.0-0.jar" +
                        " [options]");
     System.out.println("-s  <password>       Specify password");
     System.out.println(
         "-e  <encryption algorithm>       Specify encryption algorithm");
     System.out.println("-help");
   }

   /**
    * This method reads password from console
    *
    * @return CLIArgs
    */
  public
   static CLIArgs readFromConsole() {
     Console console = System.console();
     if (console == null) {
       System.err.println("Couldn't get Console instance.");
       System.exit(1);
     }
     char[] tmp1 = console.readPassword("Enter Password:");
     char[] tmp2 = console.readPassword("Re-enter Password:");
     char[] tmp3 =
         console.readPassword("Enter encryption algorithm to be used:");
     if (!Arrays.equals(tmp1, tmp2)) {
       System.err.println("Password values do not match.");
       System.exit(1);
     }
     Arrays.fill(tmp1, ' ');

     String encryptionAlgorithm = new String(tmp3).toUpperCase();
     if (tmp3.length == 0) {
       encryptionAlgorithm = EncryptionAlgorithm.RSA.name();
     } else {
       if (!EncryptionAlgorithm.isEncryptionAlgorithmValid(
                encryptionAlgorithm)) {
         System.err.println("Invalid encryption algorithm.");
         System.exit(1);
       }
     }

     CLIArgs cliArgs = new CLIArgs();
     cliArgs.setPassword(new String(tmp2));
     cliArgs.setEncryptionAlgorithm(encryptionAlgorithm);

     return cliArgs;
   }

   /**
    * This method parses command line arguments and return password
    *
    * @param args
    * @return CLIArgs
    */
  public
   static CLIArgs parseArgs(String[] args) {

     CLIArgs cliArgs = new CLIArgs();
     if (args.length > 0) {
       if (args[0] == "-help") {
         usage();
         System.exit(0);
       } else if ((args[0].equals("-s")) && (args.length == 2)) {
         cliArgs.setEncryptionAlgorithm(EncryptionAlgorithm.RSA.name());
         cliArgs.setPassword(args[1]);
       } else if ((args.length == 4) && (args[0].equals("-s")) &&
                  (args[2].equals("-e"))) {
         String encryptionAlgorithm = args[3].toUpperCase();
         if (!EncryptionAlgorithm.isEncryptionAlgorithmValid(
                  encryptionAlgorithm)) {
           System.err.println("Invalid encryption algorithm.");
           System.exit(1);
         }
         cliArgs.setEncryptionAlgorithm(encryptionAlgorithm);
         cliArgs.setPassword(args[1]);
       } else {
         logger.error("Incorrect arguments passed.");
         usage();
         System.exit(1);
       }
     }
     return cliArgs;
   }

   /**
    * This method encrypts password using public key from key store
    *
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
     String keystorePasswd = getKey(AuthEncryptConfig.getCipherUtil());

     logger.debug("Using Java Key Store File: " + s3KeystoreFile.toString());
     String certAlias = AuthEncryptConfig.getCertAlias();
     PublicKey pKey = JKSUtil.getPublicKeyFromJKS(s3KeystoreFile.toString(),
                                                  certAlias, keystorePasswd);

     if (pKey == null) {
       logger.error("Failed get public key.");
       throw new Exception("Failed get public key.");
     }

     logger.debug("Public Key :" + pKey.toString());
     encryptedPasswd = RSAEncryptDecryptUtil.encrypt(passwd, pKey);

     if (encryptedPasswd == null || encryptedPasswd.isEmpty()) {
       logger.error("Encrypted Password is null or empty");
       throw new Exception("Failed to encrypt password.");
     }
     return encryptedPasswd;
   }

   /**
    * Encrypt plain text password using AES
    *
    * @param passwd - Password to encrypt
    * @return encrypted password using AES
    * @throws Exception
    */
  public
   static String processEncryptByAesRequest(String passwd)
       throws IllegalArgumentException {
     if (passwd == null || passwd.isEmpty() || passwd.matches(".*([ \t]).*")) {
       logger.error("Password is null or empty or contains spaces");
       System.err.println("Invalid Password Value.");
       throw new IllegalArgumentException("Invalid Password Value.");
     }

     String keystorePasswd = getKey(AuthEncryptConfig.getCipherUtilGenKeyAes());

     String encryptedPasswd =
         AESEncryptDecryptUtil.encrypt(passwd, keystorePasswd);

     return encryptedPasswd;
   }

  private
   static String getKey(String cipherUtilCmd) {
     String key = null;
     try {
       Process s3Cipher = Runtime.getRuntime().exec(cipherUtilCmd);
       int exitVal = s3Cipher.waitFor();
       if (exitVal != 0) {
         logger.debug("S3 Cipher util failed to return keystore password");
         throw new IOException("S3 cipher util exited with error.");
       }
       BufferedReader reader =
           new BufferedReader(new InputStreamReader(s3Cipher.getInputStream()));
       String line = reader.readLine();
       if (line == null || line.isEmpty()) {
         throw new IOException("S3 cipher returned empty stream.");
       } else {
         key = line;
       }
     }
     catch (Exception e) {
       logger.debug(
           e.getMessage() +
           " IO error in S3 cipher. Loading default keystore credentilas.");
       key = AuthEncryptConfig.getKeyStorePassword();
     }
     return key;
   }

   /**
    * This method initializes logger
    *
    * @throws IOException
    */
   static void logInit() throws CLIInitializationException, Exception {

     AuthEncryptConfig.setLogConfigFile();
     String logConfigFilePath = AuthEncryptConfig.getLogConfigFile();

     /**
      * If log4j config file is given, override the default Logging properties
      * file.
      */
     if (logConfigFilePath != null) {
       File logConfigFile = new File(logConfigFilePath);
       if (logConfigFile.exists()) {
         ConfigurationSource source =
             new ConfigurationSource(new FileInputStream(logConfigFile));
         Configurator.initialize(null, source);
       } else {
         throw new CLIInitializationException(
             "Logging config file " + logConfigFilePath + " doesn't exist.");
       }
     }

     String logLevel = AuthEncryptConfig.getLogLevel();
     if (logLevel != null) {
       Level level = Level.getLevel(logLevel);
       if (level == null) {
         // set to ALL if couldn't find appropriate level value.
         throw new Exception("Invalid log value [" + logLevel + "]" +
                             "specified in config file");
       } else {
         Configurator.setRootLevel(level);
       }
     }
   }

  public
   static void main(String[] args) {

     try {
       AuthEncryptConfig.readConfig(AUTH_INSTALL_DIR +
                                    "/resources/keystore.properties");
     }
     catch (IOException e1) {
       logger.error("Failed to read Properties from install dir:" +
                    AUTH_INSTALL_DIR + " Error Stack :" + e1.getStackTrace());
       System.err.println("Failed to read Properties from install dir:" +
                          AUTH_INSTALL_DIR);
       System.exit(1);
     }
     try {
       logInit();
       logger = LoggerFactory.getLogger(AuthEncryptCLI.class.getName());
     }
     catch (CLIInitializationException e) {
       System.err.println("Failed to Initialize logger");
       e.printStackTrace();
       System.exit(1);
     }
     catch (Exception e) {
     }

     logger.debug("Initialization successfull");
     CLIArgs cliArgs = null;
     if (args.length == 0) {
       cliArgs = readFromConsole();
     } else {
       cliArgs = parseArgs(args);
     }
     try {
       String encryptedPasswd = "";
       switch (EncryptionAlgorithm.valueOf(cliArgs.getEncryptionAlgorithm())) {
         case RSA:
           encryptedPasswd = processEncryptRequest(cliArgs.getPassword());
           break;

         case AES:
           encryptedPasswd = processEncryptByAesRequest(cliArgs.getPassword());
           break;

         default:
           encryptedPasswd = processEncryptRequest(cliArgs.getPassword());
           break;
       }
       // Output to console
       if (encryptedPasswd == null || encryptedPasswd.isEmpty()) {
         logger.error("Failed to encrypt password due to internal error.");
         System.err.println(
             "Failed to encrypt password due to internal error.");
         System.exit(1);
       } else {
         System.out.println(encryptedPasswd);
       }
     }
     catch (GeneralSecurityException e) {
       logger.error("Failed to encrypt password. " + e.getCause() +
                    e.getMessage());
       System.err.println("Failed to encrypt password.");
       System.exit(1);
     }
     catch (Exception e) {
       logger.error("Failed to encrypt password. " + e.getCause() +
                    e.getMessage());
       System.err.println("Failed to encrypt password.");
       System.exit(1);
     }
     logger.debug("Operation completed successfully");
   }
}

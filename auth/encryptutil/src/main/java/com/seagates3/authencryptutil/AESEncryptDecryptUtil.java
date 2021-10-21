package com.seagates3.authencryptutil;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
import java.util.Base64;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class AESEncryptDecryptUtil {

 private
  static final Logger LOGGER =
      LoggerFactory.getLogger(AESEncryptDecryptUtil.class.getName());
 private
  static final String ENCRYPT_ALGO = "AES/GCM/NoPadding";
 private
  static final String SECRET_KEY_ALGO = "PBKDF2WithHmacSHA256";

 private
  static final int TAG_LENGTH_BIT = 128;
 private
  static final int IV_LENGTH_IN_BYTES = 12;
 private
  static final int SALT_LENGTH_IN_BYTES = 16;
 private
  static final Charset UTF_8 = StandardCharsets.UTF_8;

  /**
   * Encrypt plain text provided using AES
   *
   * @param plainText - Text to encrypt
   * @param password  - Const key
   * @return return a base64 encoded AES encrypted text
   */
 public
  static String encrypt(String plainText, String password) {

    String encryptedText = null;

    if (isTextNotEmpty(plainText) && isTextNotEmpty(password)) {
      byte[] salt = getRandomNonce(SALT_LENGTH_IN_BYTES);
      byte[] iv = getRandomNonce(IV_LENGTH_IN_BYTES);

      try {
        SecretKey aesKeyFromPassword =
            getAESKeyFromPassword(password.toCharArray(), salt);
        Cipher cipher = Cipher.getInstance(ENCRYPT_ALGO);
        cipher.init(Cipher.ENCRYPT_MODE, aesKeyFromPassword,
                    new GCMParameterSpec(TAG_LENGTH_BIT, iv));
        byte[] cipherText = cipher.doFinal(plainText.getBytes(UTF_8));
        byte[] cipherTextWithIvSalt =
            ByteBuffer.allocate(iv.length + salt.length + cipherText.length)
                .put(iv)
                .put(salt)
                .put(cipherText)
                .array();
        encryptedText =
            Base64.getEncoder().encodeToString(cipherTextWithIvSalt);
      }
      catch (NoSuchAlgorithmException | InvalidKeySpecException |
             NoSuchPaddingException | InvalidKeyException |
             InvalidAlgorithmParameterException | IllegalBlockSizeException |
             BadPaddingException e) {
        LOGGER.error("Error occurred while encrypting text. Cause: " +
                     e.getCause() + ". Message: " + e.getMessage());
        LOGGER.debug("Stacktrace: " + e);
      }
    }

    return encryptedText;
  }

  /**
   * Decrypt the encrypted text provided.
   *
   * @param encryptedText
   * @param password      - Const key
   * @return
   */
 public
  static String decrypt(String encryptedText, String password) {

    String decryptedText = null;

    if (isTextNotEmpty(encryptedText) && isTextNotEmpty(password)) {
      byte[] decode = Base64.getDecoder().decode(encryptedText.getBytes(UTF_8));

      ByteBuffer bb = ByteBuffer.wrap(decode);

      byte[] iv = new byte[IV_LENGTH_IN_BYTES];
      bb.get(iv);

      byte[] salt = new byte[SALT_LENGTH_IN_BYTES];
      bb.get(salt);

      byte[] cipherText = new byte[bb.remaining()];
      bb.get(cipherText);

      try {
        SecretKey aesKeyFromPassword =
            getAESKeyFromPassword(password.toCharArray(), salt);
        Cipher cipher = Cipher.getInstance(ENCRYPT_ALGO);
        cipher.init(Cipher.DECRYPT_MODE, aesKeyFromPassword,
                    new GCMParameterSpec(TAG_LENGTH_BIT, iv));

        byte[] plainText = cipher.doFinal(cipherText);

        decryptedText = new String(plainText, UTF_8);
      }
      catch (NoSuchAlgorithmException | InvalidKeySpecException |
             NoSuchPaddingException | InvalidKeyException |
             InvalidAlgorithmParameterException | IllegalBlockSizeException |
             BadPaddingException e) {
        LOGGER.error("Error occurred while decrypting the encrypted text. " +
                     "Cause: " + e.getCause() + ". Message: " + e.getMessage());
        LOGGER.debug("Stacktrace: " + e);
      }
    }

    return decryptedText;
  }

 private
  static SecretKey getAESKeyFromPassword(char[] password, byte[] salt)
      throws NoSuchAlgorithmException,
      InvalidKeySpecException {

    SecretKeyFactory factory = SecretKeyFactory.getInstance(SECRET_KEY_ALGO);
    // iterationCount = 65536
    // keyLength = 256
    KeySpec spec = new PBEKeySpec(password, salt, 65536, 256);
    SecretKey secret =
        new SecretKeySpec(factory.generateSecret(spec).getEncoded(), "AES");
    return secret;
  }

 private
  static byte[] getRandomNonce(int numBytes) {
    byte[] nonce = new byte[numBytes];
    new SecureRandom().nextBytes(nonce);
    return nonce;
  }

 private
  static boolean isTextNotEmpty(String text) {
    return text != null && !text.equals("");
  }
}

package com.seagates3.authencryptutil;
import java.security.GeneralSecurityException;

import java.security.PrivateKey;
import java.security.PublicKey;
import java.util.Base64;

import javax.crypto.Cipher;


public class RSAEncryptDecryptUtil {
    /**
     *
     * @param Method to encrypt pain text using public key
     * @param publicKey
     * @return Encrypted string
     * @throws Exception
     */
    public static String encrypt(String plainText, PublicKey publicKey)
                                 throws GeneralSecurityException, Exception {
        if (plainText == null || plainText.isEmpty()) {
           throw new Exception("Input text is null or empty");
        }
        if (publicKey == null || publicKey.toString().isEmpty()) {
           throw new Exception("Invalid Public Key");
        }
        Cipher cipher = Cipher.getInstance("RSA");
        cipher.init(Cipher.ENCRYPT_MODE, publicKey);
        return Base64.getEncoder().encodeToString(cipher.doFinal(plainText.getBytes()));
    }
    /**
     *
     * @param A method to decrypt encrypted text using private key
     * @param privateKey
     * @return plain text
     * @throws GeneralSecurityException
     */
    public static String decrypt(String encryptedText, PrivateKey privateKey)
                                 throws GeneralSecurityException, Exception{
        if (encryptedText == null || encryptedText.isEmpty()) {
            throw new Exception("Encrypted Text is null or empty");
        }
        if (privateKey == null || privateKey.toString().isEmpty()) {
             throw new Exception("Invalid Private Key");
        }
        Cipher cipher = Cipher.getInstance("RSA");
        cipher.init(Cipher.DECRYPT_MODE, privateKey);
        return new String(cipher.doFinal(Base64.getDecoder().decode(encryptedText)));
    }
}

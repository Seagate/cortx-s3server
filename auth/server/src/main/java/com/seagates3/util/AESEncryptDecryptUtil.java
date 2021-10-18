package com.seagates3.util;

import java.nio.charset.StandardCharsets;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;
import java.util.Base64;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AESEncryptDecryptUtil {

	private static final Logger LOGGER = LoggerFactory.getLogger(AESEncryptDecryptUtil.class.getName());

	/**
	 * Encrypt plain text provided using AES
	 * 
	 * @param plainText - Text to encrypt
	 * @param constKey - Const key
	 * @param salt - Salt
	 * @return
	 */
	public static String encrypt(String plainText, String constKey, String salt) {

		String encryptedText = null;
		IvParameterSpec ivspec = generateDefaultIv();

		if (plainText != null && !plainText.isEmpty()) {
			try {
				SecretKey secretKey = getAESKeyFromConstKey(constKey, salt);
				Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
				cipher.init(Cipher.ENCRYPT_MODE, secretKey, ivspec);
				encryptedText = Base64.getEncoder()
						.encodeToString(cipher.doFinal(plainText.getBytes(StandardCharsets.UTF_8)));
			} catch (NoSuchAlgorithmException | InvalidKeySpecException | NoSuchPaddingException | InvalidKeyException
					| InvalidAlgorithmParameterException | IllegalBlockSizeException | BadPaddingException e) {
				LOGGER.error("Error occurred while encrypting text. Cause: " + e.getCause() + ". Message: "
						+ e.getMessage());
				LOGGER.debug("Stacktrace: " + e);
			}
		}

		return encryptedText;
	}

	/**
	 * Decrypt the encrypted text provided.
	 * 
	 * @param encryptedText
	 * @param constKey
	 * @param salt
	 * @return
	 */
	public static String decrypt(String encryptedText, String constKey, String salt) {

		String decryptedText = null;
		if (encryptedText != null && !encryptedText.isEmpty()) {
			try {
				IvParameterSpec ivspec = generateDefaultIv();
				SecretKey secretKey = getAESKeyFromConstKey(constKey, salt);

				Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5PADDING");
				cipher.init(Cipher.DECRYPT_MODE, secretKey, ivspec);
				decryptedText = new String(cipher.doFinal(Base64.getDecoder().decode(encryptedText)));
			} catch (NoSuchAlgorithmException | InvalidKeySpecException | NoSuchPaddingException | InvalidKeyException
					| InvalidAlgorithmParameterException | IllegalBlockSizeException | BadPaddingException e) {
				LOGGER.error("Error occurred while decrypting text. Cause: " + e.getCause() + ". Message: "
						+ e.getMessage());
				LOGGER.debug("Stacktrace: " + e);
			}
		}

		return decryptedText;
	}

	/**
	 * Build AES const key
	 * 
	 * @param constKey
	 * @param salt
	 * @return
	 * @throws NoSuchAlgorithmException
	 * @throws InvalidKeySpecException
	 */
	private static SecretKey getAESKeyFromConstKey(String constKey, String salt)
			throws NoSuchAlgorithmException, InvalidKeySpecException {

		SecretKeyFactory factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");

		KeySpec spec = new PBEKeySpec(constKey.toCharArray(), salt.getBytes(), 65536, 256);
		SecretKey tmp = factory.generateSecret(spec);
		SecretKey secretKey = new SecretKeySpec(tmp.getEncoded(), "AES");
		return secretKey;
	}

	/**
	 * Build default IvParameterSpec
	 * @return
	 */
	private static IvParameterSpec generateDefaultIv() {
		byte[] iv = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		IvParameterSpec ivspec = new IvParameterSpec(iv);
		return ivspec;
	}
}

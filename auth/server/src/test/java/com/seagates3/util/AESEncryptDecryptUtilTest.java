package com.seagates3.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class AESEncryptDecryptUtilTest {
	private String password = "Seagate@1";
	private String const_key = "cortx";
	private String salt = "cluster_id_1";

	@Test
	public void testEncryptDecrypt() {

		new AESEncryptDecryptUtil();

		String encryptedText = AESEncryptDecryptUtil.encrypt(password, const_key, salt);
		assertNotNull(encryptedText);
		assertFalse(password.equals(encryptedText));

		String plainText = AESEncryptDecryptUtil.decrypt(encryptedText, const_key, salt);
		assertNotNull(plainText);
		assertTrue(plainText.equals(password));
	}

	@Test
	public void testNegativeEncryptDecrypt() {

		String encryptedText1 = AESEncryptDecryptUtil.encrypt(null, const_key, salt);
		String encryptedText2 = AESEncryptDecryptUtil.encrypt("", const_key, salt);

		String decryptedText1 = AESEncryptDecryptUtil.decrypt(null, const_key, salt);
		String decryptedText2 = AESEncryptDecryptUtil.decrypt("", const_key, salt);

		assertNull(encryptedText1);
		assertNull(encryptedText2);
		assertNull(decryptedText1);
		assertNull(decryptedText2);

		String encryptedText3 = AESEncryptDecryptUtil.encrypt(password, const_key, salt);
		String decryptedText3 = AESEncryptDecryptUtil.decrypt(encryptedText3, const_key, "test");

		assertNull(decryptedText3);

		String encryptedText4 = AESEncryptDecryptUtil.encrypt(password, "", salt);

		assertNull(encryptedText4);

	}

}

package com.seagates3.authencryptutil;

public enum EncryptionAlgorithm {

	AES, RSA;
	
	public static boolean isEncryptionAlgorithmValid(String encryptionAlgorithm) {
		boolean isValid = true;
		try {
			valueOf(encryptionAlgorithm);
		} catch (IllegalArgumentException ex) {
			isValid = false;
		}
		return isValid;
	}
}

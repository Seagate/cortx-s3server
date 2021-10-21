package com.seagates3.authpassencryptcli;

public
class CLIArgs {

 private
  String password;
 private
  String encryptionAlgorithm;

 public
  String getPassword() { return password; }

 public
  void setPassword(String password) { this.password = password; }

 public
  String getEncryptionAlgorithm() { return encryptionAlgorithm; }

 public
  void setEncryptionAlgorithm(String encryptionAlgorithm) {
    this.encryptionAlgorithm = encryptionAlgorithm;
  }
}

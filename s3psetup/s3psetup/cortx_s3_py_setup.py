#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.

import argparse
import sys
import os
import subprocess
from s3backgrounddelete.cortx_s3_cipher import CortxS3Cipher

class S3CortxSetup:

  def __init__(self):
    """Instantiate S3CortxSetup"""

  def delete_background_delete_account(self, ldappasswd: str, keylen: int, key: str, s3background_cofig:str):
    """Delete s3 account which was used by s3background delete"""
    cmd = 'ldapsearch -b "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -x -w ' + ldappasswd + ' -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://'
    print ("1: {}".format(cmd))
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("2: {} {}".format(output, error))
    outputs = output.split(" ")
    print ("3: {} ".format(outputs))
    for substr in outputs :
      if substr == "No such object":
        print ("4: Nothing found, {}".format(output))
        return True
    """Delete s3background delete account"""
    s3_cipher = CortxS3Cipher(None, True, keylen, key)
    access_key = ""
    try:
      access_key = s3_cipher.get_key()
    except CipherInvalidToken as err:
      print("Cipher generate key failed with error : {0}, trying from flat file : {1}".format(err, s3background_cofig))
      cmd = "awk '/background_account_access_key/ {print}' "+ s3background_cofig + " | cut -d " " -f 5 | sed -e 's/^\"//' -e 's/\"$//"
      print ("5: {}".format(cmd))
      access_key,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
      print ("6: {} {}".format(access_key, error))
    cmd = "ldapdelete -x -w " + ldappasswd + " -r \"ak=" + access_key + ",ou=accesskeys,dc=s3,dc=seagate,dc=com\" -D \"cn=sgiamadmin,dc=seagate,dc=com\" -H ldapi:///"
    print ("7: {}".format(cmd))
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("8: {} {}".format(output, error))
    cmd = 'ldapdelete -x -w ' + ldappasswd + ' -r "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///'
    print ("9: {}".format(cmd))
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("10: {} {}".format(output, error))
    print ("11: Deleted s3backgrounddelete account successfully...")
    return True 

  def delete_recovery_tool_account(self, ldappasswd: str, keylen: int, key: str, s3background_cofig:str):
    """Delete s3 account which was used by s3recoverytool"""
    cmd = 'ldapsearch -b "o=s3-recovery-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -x -w ' + ldappasswd + ' -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://'
    print ("12: {}".format(cmd))
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("13: {} {}".format(output, error))
    outputs = output.split(" ")
    for substr in outputs :
      if substr == "No such object":
        print ("14: Nothing found, {}".format(output))
        return True
    """Delete s3background delete account"""
    s3_cipher = CortxS3Cipher(None, True, keylen, key)
    access_key = ""
    try:
      access_key = s3_cipher.get_key()
    except CipherInvalidToken as err:
      print("Cipher generate key failed with error : {0}, trying from flat file : {1}".format(err, s3background_cofig))
      cmd = "awk '/background_account_access_key/ {print}' "+ s3background_cofig + " | cut -d " " -f 5 | sed -e 's/^\"//' -e 's/\"$//"
      print ("15: {}".format(cmd))
      access_key,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
      print ("16: {} {}".format(access_key, error))
    cmd = 'ldapdelete -x -w ' + ldappasswd + ' -r "ak=' + access_key + ',ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///'
    print ("17: {}".format(cmd))
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("18: {} {}".format(output, error))
    cmd = 'ldapdelete -x -w ' + ldappasswd + ' -r "o=s3-recovery-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///'
    print ("19: {}".format(cmd))
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("20: {} {}".format(output, error))
    print ("21: Deleted s3recovery tool account successfully...")
    return True 

  def accounts_cleanup(self, ldappasswd: str, s3background_cofig:str = "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml"):
    """Clean up s3 accounts"""
    self.delete_background_delete_account(ldappasswd, 22, "s3backgroundaccesskey", s3background_cofig)
    self.delete_recovery_tool_account(ldappasswd, 22, "s3recoveryaccesskey", s3background_cofig)

  def dependencies_cleanup(self):
    """Clean up s3 dependencies"""
    print ("removing s3 dependencies")
    cmd = "yum -y remove log4cxx_cortx"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("{} {}".format(output, error))
    cmd = "mv -f /opt/seagate/cortx/auth/resources/authserver.properties"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("{} {}".format(output, error))
    cmd = "mv -f /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("{} {}".format(output, error))
    cmd = "mv -f /opt/seagate/cortx/auth/resources/keystore.properties /opt/seagate/cortx/auth/resources/keystore.properties.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("{} {}".format(output, error))
    cmd = "mv -f /opt/seagate/cortx/s3/conf/s3config.yaml /opt/seagate/cortx/s3/conf/s3config.yaml.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    print ("{} {}".format(output, error))

  def run(self):
    parser = argparse.ArgumentParser(description='Cortx S3 Setup')
    parser.add_argument("--ldappasswd", help='ldap password, needed for --cleanup')
    parser.add_argument("--cleanup", help='Cleanup S3 accounts and dependencies, pass Both, else either accounts or dependencies.')
    """Future functionalities to be added here."""

    args = parser.parse_args()
    if args.cleanup != None:
      if args.ldappasswd == None:
        print("Please provide --ldappasswd")
        exit(-2)

      if args.cleanup == "accounts":
        self.accounts_cleanup(args.ldappasswd)
      elif args.cleanup == "dependencies":
        self.dependencies_cleanup(args.ldappasswd)
      elif args.cleanup == "both":
        self.accounts_cleanup(args.ldappasswd)
        self.dependencies_cleanup()
      else:
        print("Invalid input for cleanup {}".format(args.cleanup))
        exit (-2)

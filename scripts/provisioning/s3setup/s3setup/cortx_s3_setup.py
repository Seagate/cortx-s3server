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
import logging
import re

logging.basicConfig(level=logging.INFO)
"""logging.basicConfig(level=logging.DEBUG)"""
log=logging.getLogger('=>')    

class S3CortxSetup:

  def __init__(self):
    """Instantiate S3CortxSetup"""

  def delete_background_delete_account(self, ldappasswd: str, keylen: int, key: str, s3background_cofig:str):
    """Delete s3 account which was used by s3background delete"""
    cmd = 'ldapsearch -b "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -x -w ' + ldappasswd + ' -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://'
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    output = re.sub(r"[\n\t\s]*","",output)
    if "result:32Nosuchobjectmatched" in output:
      print("No s3-background-delete-svc account found")
      return False
    """Delete s3background delete account"""
    s3_cipher = CortxS3Cipher(None, True, keylen, key)
    access_key = ""
    try:
      access_key = s3_cipher.get_key()
    except CipherInvalidToken as err:
      log.debug("Cipher generate key failed with error : {0}, trying from flat file : {1}".format(err, s3background_cofig))
      cmd = "awk '/background_account_access_key/ {print}' "+ s3background_cofig + " | cut -d " " -f 5 | sed -e 's/^\"//' -e 's/\"$//"
      access_key,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
      log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, access_key, error))
    cmd = "ldapdelete -x -w " + ldappasswd + " -r \"ak=" + access_key + ",ou=accesskeys,dc=s3,dc=seagate,dc=com\" -D \"cn=sgiamadmin,dc=seagate,dc=com\" -H ldapi:///"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    cmd = 'ldapdelete -x -w ' + ldappasswd + ' -r "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///'
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    if not error:
      print ("Deleted s3backgrounddelete account successfully...")
      return True 
    print ("Delete s3backgrounddelete account failed with: {}".format(error))
    return False

  def delete_recovery_tool_account(self, ldappasswd: str, keylen: int, key: str, s3background_cofig:str):
    """Delete s3 account which was used by s3recoverytool"""
    cmd = 'ldapsearch -b "o=s3-recovery-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -x -w ' + ldappasswd + ' -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://'
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    output = re.sub(r"[\n\t\s]*","",output)
    if "result:32Nosuchobjectmatched" in output:
      print("No s3-recovery-svc account found")
      return False
    """Delete s3recovery tool account"""
    s3_cipher = CortxS3Cipher(None, True, keylen, key)
    access_key = ""
    try:
      access_key = s3_cipher.get_key()
    except CipherInvalidToken as err:
      log.debug("Cipher generate key failed with error : {0}, trying from flat file : {1}".format(err, s3background_cofig))
      cmd = "awk '/background_account_access_key/ {print}' "+ s3background_cofig + " | cut -d " " -f 5 | sed -e 's/^\"//' -e 's/\"$//"
      access_key,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
      log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, access_key, error))
    cmd = 'ldapdelete -x -w ' + ldappasswd + ' -r "ak=' + access_key + ',ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///'
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    cmd = 'ldapdelete -x -w ' + ldappasswd + ' -r "o=s3-recovery-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///'
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    if not error:
      print ("Deleted s3recovery tool account successfully...")
      return True 
    print ("Delete s3recovery tool account failed with: {}".format(error))
    return False

  def accounts_cleanup(self, ldappasswd: str = None, s3background_cofig:str = "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml"):
    """Clean up s3 accounts"""
    if ldappasswd == None:
      S3Cipher = CortxS3Cipher(None, True, 12, "openldap")
      ldappasswd = S3Cipher.get_key()
    rc1 = self.delete_background_delete_account(ldappasswd, 22, "s3backgroundaccesskey", s3background_cofig)
    rc2 = self.delete_recovery_tool_account(ldappasswd, 22, "s3recoveryaccesskey", s3background_cofig)
    return rc1 & rc2

  def dependencies_cleanup(self):
    """Clean up configs"""
    log.debug("removing s3 dependencies")
    cmd = "mv -f /opt/seagate/cortx/auth/resources/authserver.properties /opt/seagate/cortx/auth/resources/authserver.properties.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    cmd = "mv -f /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    cmd = "mv -f /opt/seagate/cortx/auth/resources/keystore.properties /opt/seagate/cortx/auth/resources/keystore.properties.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    cmd = "mv -f /opt/seagate/cortx/s3/conf/s3config.yaml /opt/seagate/cortx/s3/conf/s3config.yaml.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    cmd = "mv -f /opt/seagate/cortx/s3/conf/s3_confstore.json /opt/seagate/cortx/s3/conf/s3_confstore.json.bak"
    output,error = subprocess.Popen(cmd, universal_newlines=True, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    log.debug("\ncmd:{0},\noutput:{1},\nerror:{2}".format(cmd, output, error))
    return True

  def run(self):
    parser = argparse.ArgumentParser(description='Cortx S3 Setup')
    parser.add_argument("--cleanup", help='Cleanup S3 accounts and dependencies. Valid values: all/accounts/dependencies')
    """Future functionalities to be added here."""
    parser.add_argument("--ldappasswd", help='Optional ldap password, needed for --cleanup')

    args = parser.parse_args()

    if args.cleanup != None:
      if args.cleanup == "accounts":
        if args.ldappasswd: 
          rc = self.accounts_cleanup(args.ldappasswd)
        else:
          rc = self.accounts_cleanup()
        exit (not rc)
      elif args.cleanup == "dependencies":
        rc = self.dependencies_cleanup()
        exit (not rc)
      elif args.cleanup == "all":
        if args.ldappasswd: 
          rc1 = self.accounts_cleanup(args.ldappasswd)
        else:
          rc1 = self.accounts_cleanup()
        rc2 = self.dependencies_cleanup()
        exit (not (rc1 & rc2))
      else:
        print("Invalid input for cleanup {}. Valid values: all/accounts/dependencies".format(args.cleanup))
        exit (-2)

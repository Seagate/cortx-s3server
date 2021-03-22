#!/usr/bin/env python3
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
#

import sys
import os
import shutil
import glob

from ldapaccountaction import LdapAccountAction
from setupcmd import SetupCmd

class ResetCmd(SetupCmd):
  """Reset Setup Cmd."""
  name = "reset"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(ResetCmd, self).__init__(config)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")
    try:
      sys.stdout.write('INFO: Removing LDAP Accounts and Users.\n')
      self.DeleteLdapAccountsUsers()
      sys.stdout.write('INFO:LDAP Accounts and Users Cleanup successful.\n')
    except Exception as e:
      sys.stderr.write(f'Failed to cleanup LDAP Accounts and Users, error: {e}\n')
      raise e
    try:
      sys.stdout.write('INFO: Cleaning up log files.\n')
      self.CleanupLogs()
      sys.stdout.write('INFO:Log files cleanup successful.\n')
    except Exception as e:
      sys.stderr.write(f'Failed to cleanup log directories or files, error: {e}\n')
      raise e
    

  def CleanupLogs(self):
    """Cleanup all the log directories and files."""
    # Backgrounddelete -> /var/log/seagate/s3/s3backgrounddelete/
    #Audit -> /var/log/seagate/s3/audit
    # s3 -> /var/log/seagate/s3/
    #Auth -> /var/log/seagate/auth/
    #S3 Motr -> /var/log/seagate/motr/s3server-*
    #HAproxy -> /var/log/haproxy.log
    #HAproxy -> /var/log/haproxy-status.log
    #Slapd -> /var/log/slapd.log
    #S3 Crash dumps -> /var/log/crash/core-s3server.*.gz

    logFolders = ["/var/log/seagate/s3",
                  "/var/log/seagate/auth"]

    for logFolder in logFolders:
      self.DeleteDirContents(logFolder)

    logFiles = ["/var/log/haproxy.log",
                "/var/log/haproxy-status.log",
                "/var/log/slapd.log"]
    for logFile in logFiles:
      self.DeleteFile(logFile)

    # logRegexPath represents key->path and value->regex
    logRegexPath =  { '/var/log/seagate/motr':'s3server-*',
                      '/var/log/crash':'core-s3server.*.gz'}
    for path in logRegexPath:
      self.DeleteFileOrDirWithRegex(path, logRegexPath[path])

  def DeleteDirContents(self, dirname: str):
    """Delete files and directories inside given directory."""
    if os.path.exists(dirname):
      for filename in os.listdir(dirname):
        filepath = os.path.join(dirname, filename)
        try:
          if os.path.isfile(filepath):
            os.remove(filepath)
          elif os.path.isdir(filepath):
            shutil.rmtree(filepath)
        except Exception as e:
          sys.stderr.write(f'ERROR: DeleteDirContents(): Failed to delete: {filepath}, error: {str(e)}\n')
          raise e

  def DeleteFile(self, filepath: str):
    """Delete file."""
    if os.path.exists(filepath):
      try:
        os.remove(filepath)
      except Exception as e:
        sys.stderr.write(f'ERROR: DeleteFile(): Failed to delete file: {filepath}, error: {str(e)}\n')
        raise e

  def DeleteFileOrDirWithRegex(self, path: str, regex: str):
    """Delete files and directories inside given directory for which regex matches."""
    if os.path.exists(path):
      filepath = os.path.join(path, regex)
      files = glob.glob(filepath)
      for file in files:
        try:
          if os.path.isfile(file):
            os.remove(file)
          elif os.path.isdir(file):
            shutil.rmtree(file)
        except Exception as e:
          sys.stderr.write(f'ERROR: DeleteFileOrDirWithRegex(): Failed to delete: {file}, error: {str(e)}\n')
          raise e

  def DeleteLdapAccountsUsers(self):
    """Deletes all LDAP accounts and users"""
    os.system('slapcat -n 3 -l /tmp/conf_backup.ldif')
    os.system('sed -i \'118,$ d\' /tmp/conf_backup.ldif')
    ldap_mdb_folder = "/var/lib/ldap"
    for files in os.listdir(ldap_mdb_folder):
      path = os.path.join(ldap_mdb_folder,files)
      if os.path.isfile(path) or os.path.islink(path):
        os.unlink(path)
      elif os.path.isdir(path):
        shutil.rmtree(path)
    os.system('systemctl restart slapd')
    os.system('slapadd -n 3 -F /etc/openldap/slapd.d -l /tmp/conf_backup.ldif')
    try:
      os.remove('/tmp/conf_backup.ldif')
    except Exception as e:
      sys.stderr.write(f'ERROR: No such file! , error: {str(e)}\n')
      raise e
    try:
      # Recreate background delete account after LDAP reset
      self.read_ldap_credentials()
      bgdelete_acc_input_params_dict = {'account_name': "s3-background-delete-svc",
                                  'account_id': "67891",
                                  'canonical_id': "C67891",
                                  'mail': "s3-background-delete-svc@seagate.com",
                                  's3_user_id': "450",
                                  'const_cipher_secret_str': "s3backgroundsecretkey",
                                  'const_cipher_access_str': "s3backgroundaccesskey"
                                }
      LdapAccountAction(self.ldap_user, self.ldap_passwd).create_account(bgdelete_acc_input_params_dict)
    except Exception as e:
      sys.stderr.write(f'Failed to create backgrounddelete service account, error: {e}\n')
      raise e

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
import time
import re
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_constants import MESSAGE_BUS
from setupcmd import SetupCmd
from ldapaccountaction import LdapAccountAction

services_list = ["haproxy", "s3backgroundproducer", "s3backgroundconsumer", "s3server@*", "s3authserver", "slapd"]

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
    self.logger.info(f"Processing {self.name} {self.url}\n")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)

    try:
      self.logger.info(' Removing LDAP Accounts and Users.\n')
      self.DeleteLdapAccountsUsers()
      self.logger.info(' LDAP Accounts and Users Cleanup successful.\n')
    except Exception as e:
      self.logger.error(f'ERROR:Failed to cleanup LDAP Accounts and Users, error: {e}\n')
      raise e

    try:
      self.logger.info('Shutting down s3 services...\n")
      self.shutdown_services(services_list)
    except Exception as e:
      self.logger.error(f'ERROR:Failed to stop s3services, error: {e}\n')
      raise e

    try:
      self.logger.info(' Cleaning up log files.\n')
      self.CleanupLogs()
      self.logger.info('Log files cleanup successful.\n')

      # purge messages from message bus
      bgdeleteconfig = CORTXS3Config()
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        self.logger.info(' Purging messages from message bus.\n')
        self.purge_messages(bgdeleteconfig.get_msgbus_producer_id(),
                            bgdeleteconfig.get_msgbus_topic(),
                            bgdeleteconfig.get_msgbus_producer_delivery_mechanism(),
                            bgdeleteconfig.get_purge_sleep_time())
        self.logger.info('Purge message successful.\n')

    except Exception as e:
      self.logger.error(f'ERROR: Failed to cleanup log directories or files, error: {e}\n')
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
          self.logger.error(f'ERROR: DeleteDirContents(): Failed to delete: {filepath}, error: {str(e)}\n')
          raise e

  def DeleteFile(self, filepath: str):
    """Delete file."""
    if os.path.exists(filepath):
      try:
        os.remove(filepath)
      except Exception as e:
        self.logger.error(f'ERROR: DeleteFile(): Failed to delete file: {filepath}, error: {str(e)}\n')
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
          self.logger.error(f'ERROR: DeleteFileOrDirWithRegex(): Failed to delete: {file}, error: {str(e)}\n')
          raise e

  def purge_messages(self, producer_id: str, msg_type: str, delivery_mechanism: str, sleep_time: int):
    """purge messages on message bus."""
    try:
      s3MessageBus = S3CortxMsgBus()
      s3MessageBus.setup_producer(producer_id, msg_type, delivery_mechanism)
      try:
        s3MessageBus.purge()
        #Insert a delay of 1 min after purge, so that the messages are deleted
        time.sleep(sleep_time)
      except:
        self.logger.info('Exception during purge. May be there are no messages to purge\n')
    except Exception as e:
      raise e

  def DeleteLdapAccountsUsers(self):
    """Deletes all LDAP data entries e.g. accounts, users, access keys using admin credentials."""
    self.read_ldap_credentials()

    try:
      # Delete data directories e.g. ou=accesskeys, ou=accounts,ou=idp from dc=s3,dc=seagate,dc=com tree"
      LdapAccountAction(self.ldap_root_user, self.rootdn_passwd).delete_s3_ldap_data()
    except Exception as e:
      self.logger.error(f'ERROR: Failed to delete s3 recoards exists in ldap, error: {e}\n')
      raise e

    try:
      # Recreate background delete account after LDAP reset
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
      self.logger.error(f'ERROR: Failed to create backgrounddelete service account, error: {e}\n')
      raise e

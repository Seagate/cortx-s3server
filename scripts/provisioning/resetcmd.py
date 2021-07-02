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
      self.get_ldap_root_credentials()
      self.get_iam_admin_credentials()
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing {self.name}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.validate_config_files(self.name)
    self.logger.info("validations completed")

    try:
      self.logger.info('Remove LDAP Accounts and Users started')
      self.DeleteLdapAccountsUsers()
      self.logger.info('Remove LDAP Accounts and Users completed')
    except Exception as e:
      self.logger.error(f'ERROR:Failed to cleanup LDAP Accounts and Users, error: {e}')
      raise e

    try:
      self.logger.info("Shutdown S3 services started")
      self.shutdown_services(services_list)
      self.logger.info("Shutdown S3 services completed")
    except Exception as e:
      self.logger.error(f'ERROR:Failed to stop s3services, error: {e}')
      raise e

    try:
      self.logger.info('Cleanup log file started')
      self.CleanupLogs()
      self.logger.info('Cleanup log file completed')

      # purge messages from message bus
      bgdeleteconfig = CORTXS3Config()
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        self.logger.info('purge messages from message bus started')
        self.purge_messages(bgdeleteconfig.get_msgbus_producer_id(),
                            bgdeleteconfig.get_msgbus_topic(),
                            bgdeleteconfig.get_msgbus_producer_delivery_mechanism(),
                            bgdeleteconfig.get_purge_sleep_time())
        self.logger.info('purge messages from message bus completed')

    except Exception as e:
      self.logger.error(f'ERROR: Failed to cleanup log directories or files, error: {e}')
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

    logDirs = ["/var/log/seagate/s3",
                  "/var/log/seagate/auth"]
    # Skipping s3deployment.log file directory as we dont need to remove it as part of log cleanup
    skipDirs = ["/var/log/seagate/s3/s3deployment"]

    for logDir in logDirs:
      self.DeleteDirContents(logDir, skipDirs)

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
        self.logger.info('Exception during purge. May be there are no messages to purge')
    except Exception as e:
      raise e

  def DeleteLdapAccountsUsers(self):
    """Deletes all LDAP data entries e.g. accounts, users, access keys using admin credentials."""
    try:
      # Delete data directories e.g. ou=accesskeys, ou=accounts,ou=idp from dc=s3,dc=seagate,dc=com tree"
      LdapAccountAction(self.ldap_root_user, self.rootdn_passwd).delete_s3_ldap_data()
    except Exception as e:
      self.logger.error(f'ERROR: Failed to delete s3 records exists in ldap, error: {e}')
      raise e

    try:
      # Recreate background delete account after LDAP reset
      bgdelete_acc_input_params_dict = self.create_dict_for_BG_delete_account()
      LdapAccountAction(self.ldap_user, self.ldap_passwd).create_account(bgdelete_acc_input_params_dict)
    except Exception as e:
      if "Already exists" not in str(e):
        self.logger.error(f'Failed to create backgrounddelete service account, error: {e}')
        raise(e)
      else:
        self.logger.warning("backgrounddelete service account already exist")

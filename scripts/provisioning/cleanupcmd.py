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
from pathlib import Path
import shutil

from setupcmd import SetupCmd, S3PROVError
from ldapaccountaction import LdapAccountAction
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_constants import MESSAGE_BUS
from cortx.utils.validator.error import VError

class CleanupCmd(SetupCmd):
  """Cleanup Setup Cmd."""
  name = "cleanup"
  # map with key as 'account-name', and value as the account related constants
  account_cleanup_dict = {
                          "s3-background-delete-svc": {
                            "s3userId": "450"
                          }
                        }
  # ldap config and schema related constants
  ldap_configs = {
                  "files": [
                             "/etc/openldap/slapd.d/cn=config/cn=schema/cn={2}s3user.ldif"
                           ]
                 }

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(CleanupCmd, self).__init__(config)
      self.get_iam_admin_credentials()

    except Exception as e:
      self.logger.error(f'Failed to read ldap credentials, error: {e}')
      raise e

  def process(self, pre_factory = False):
    """Main processing function."""
    self.logger.info(f"Processing {self.name}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.validate_config_files(self.name)
    self.logger.info("validations completed")

    try:
      self.logger.info("checking if ldap service is running or not...")
      self.validate_pre_requisites(rpms=None, pip3s=None, services="slapd", files=None)
    except VError as e:
      self.logger.info("slapd service is not running hence starting it...")
      service_list = ["slapd"]
      self.start_services(service_list)
    except Exception as e:
      self.logger.error(f'Failed to validate/restart slapd, error: {e}')
      raise e
    self.logger.info("slapd service is running...")

    try:
      # Check if reset phase was performed before this
      self.detect_if_reset_done()

      # cleanup ldap accounts related to S3
      self.logger.info("delete ldap account of S3 started")
      ldap_action_obj = LdapAccountAction(self.ldap_user, self.ldap_passwd)
      ldap_action_obj.delete_account(self.account_cleanup_dict)
      self.logger.info("delete ldap account of S3 completed")

      # Erase haproxy configurations
      self.logger.info("erase haproxy configuration started")
      self.cleanup_haproxy_configurations()
      self.logger.info("erase haproxy configuration completed")

      # cleanup ldap config and schemas
      self.logger.info("delete ldap config and schemas started")
      self.delete_ldap_config()
      self.logger.info("delete ldap config and schemas completed")

      # Delete topic created for background delete
      bgdeleteconfig = CORTXS3Config()
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        self.logger.info('Deleting topic started')
        self.delete_topic(bgdeleteconfig.get_msgbus_admin_id, bgdeleteconfig.get_msgbus_topic())
        self.logger.info('Deleting topic completed')

      #delete deployment log
      if pre_factory == True:
        self.logger.info("Delete S3 Deployment log file started")
        dirpath = "/var/log/seagate/s3/s3deployment"
        self.DeleteDirContents(dirpath)
        self.logger.info("Delete S3 Deployment log file completed")
        # revert config files to their origional config state
        self.logger.info('revert s3 config files started')
        self.revert_config_files()
        self.logger.info('revert s3 config files completed')
      else:
        self.logger.info("Skipped Delete of S3 Deployment log file")

      service_list = ["slapd","haproxy"]
      self.restart_services(service_list) 

    except Exception as e:
      raise e

  def revert_config_files(self):
    """Revert config files to their original config state."""

    configFiles = ["/opt/seagate/cortx/auth/resources/authserver.properties",
                  "/opt/seagate/cortx/auth/resources/keystore.properties",
                  "/opt/seagate/cortx/s3/conf/s3config.yaml",
                  "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml",
                  "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml"]
    try:
      for configFile in configFiles:
        if os.path.isfile(configFile):
            shutil.copy(configFile+".sample", configFile)

      #Handling jks template separately as it does not have .sample extention
      auth_jksstore = "/opt/seagate/cortx/auth/resources/s3authserver.jks"
      auth_jksstore_template ="/opt/seagate/cortx/auth/scripts/s3authserver.jks_template"
      if os.path.isfile(auth_jksstore):
        shutil.copy(auth_jksstore_template, auth_jksstore)

    except Exception as e:
      self.logger.error(f'Failed to revert config files in Cleanup phase, error: {e}')
      raise e

  def cleanup_haproxy_configurations(self):
    """Resetting haproxy config."""
    #Initialize header and footer text
    header_text = "#-------S3 Haproxy configuration start---------------------------------"
    footer_text = "#-------S3 Haproxy configuration end-----------------------------------"

    #Remove all existing content from header to footer
    is_found = False
    header_found = False
    original_file = "/etc/haproxy/haproxy.cfg"
    dummy_file = original_file + '.bak'
    # Open original file in read only mode and dummy file in write mode
    with open(original_file, 'r') as read_obj, open(dummy_file, 'w') as write_obj:
        # Line by line copy data from original file to dummy file
        for line in read_obj:
            line_to_match = line
            if line[-1] == '\n':
                line_to_match = line[:-1]
            # if current line matches with the given line then skip that line
            if line_to_match != header_text:
                if header_found == False:
                    write_obj.write(line)
                elif line_to_match == footer_text:
                     header_found = False
            else:
                is_found = True
                header_found = True
                break
    read_obj.close()
    write_obj.close()
    # If any line is skipped then rename dummy file as original file
    if is_found:
        os.remove(original_file)
        os.rename(dummy_file, original_file)
    else:
        os.remove(dummy_file)

  def detect_if_reset_done(self):
    """Validate if reset phase has done or not, throw exception."""
    self.logger.info(f"Processing {self.name} detect_if_reset_done")
    # Validate log file cleanup.
    log_files = ['/var/log/seagate/auth/server/app.log', '/var/log/seagate/s3/s3server-*/s3server.INFO']
    for fpath in log_files:
      if os.path.exists(fpath):
        raise S3PROVError("Stale log files found in system!!!! hence reset needs to be performed before cleanup can be processed")
    # Validate ldap entry cleanup.
    self.validate_ldap_account_cleanup()
    self.logger.info(f"Processing {self.name} detect_if_reset_done completed successfully..")

  def validate_ldap_account_cleanup(self):
    """Validate ldap data is cleaned."""
    account_count=0
    try :
      self.logger.info("Validating ldap account entries")
      ldap_action_obj = LdapAccountAction(self.ldap_user, self.ldap_passwd)
      account_count = ldap_action_obj.get_account_count()
    except Exception as e:
      self.logger.error(f"ERROR: Failed to find total count of ldap account, error: {str(e)}")
      raise e
    if account_count > 1:
      raise S3PROVError("Stale account entries found in ldap !!!! hence reset needs to be performed before cleanup can be processed")
    self.logger.info("Validation of ldap account entries successful.")

  def delete_ldap_config(self):
    """Delete the ldap configs created by setup_ldap.sh during config phase."""
    # Clean up old configuration if any
    # Removing schemas
    files = self.ldap_configs["files"]
    for curr_file in files:
      if os.path.isfile(curr_file):
        os.remove(curr_file)
        self.logger.info(f"{curr_file} removed")
    # Delete s3 specific slapd indices and revert to original basic
    self.modify_attribute('olcDatabase={2}mdb,cn=config', 'olcDbIndex', 'ou,cn,mail,surname,givenname eq,pres,sub')
    # Delete s3 specific olcAccess
    self.search_and_delete_attribute("olcDatabase={2}mdb,cn=config", "olcAccess")

  def delete_topic(self, admin_id, topic_name):
    """delete topic for background delete services."""
    try:
      if S3CortxMsgBus.is_topic_exist(admin_id, topic_name):
        S3CortxMsgBus.delete_topic(admin_id, [topic_name])
      else:
        self.logger.info("Topic does not exist")
    except Exception as e:
      raise e

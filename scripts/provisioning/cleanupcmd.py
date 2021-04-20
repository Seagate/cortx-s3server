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
                             "/etc/openldap/slapd.d/cn=config/cn=schema/cn={1}s3user.ldif",
                             "/etc/openldap/slapd.d/cn=config/cn=module{0}.ldif",
                             "/etc/openldap/slapd.d/cn=config/cn=module{1}.ldif",
                             "/etc/openldap/slapd.d/cn=config/olcDatabase={2}mdb.ldif"
                          ],
                  "files_wild": [
                             {
                              "path": "/etc/openldap/slapd.d/cn=config/cn=schema/",
                              "glob": "*ppolicy.ldif"
                             }
                          ],
                  "dirs": [
                             "/etc/openldap/slapd.d/cn=config/olcDatabase={2}mdb"
                          ]
                 }

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(CleanupCmd, self).__init__(config)

      self.read_ldap_credentials()
    except Exception as e:
      sys.stderr.write(f'Failed to read ldap credentials, error: {e}\n')
      raise e

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)

    try:
      sys.stdout.write("checking if ldap service is running or not...\n")
      self.validate_pre_requisites(rpms=None, pip3s=None, services="slapd", files=None)
    except VError as e:
      sys.stdout.write("slapd service is not running hence starting it...\n")
      service_list = ["slapd"]
      self.start_services(service_list)
    except Exception as e:
      sys.stderr.write(f'Failed to validate/restart slapd, error: {e}\n')
      raise e
    sys.stdout.write("slapd service is running...\n")

    try:
      # Check if reset phase was performed before this
      self.detect_if_reset_done()

      # cleanup ldap accounts related to S3
      ldap_action_obj = LdapAccountAction(self.ldap_user, self.ldap_passwd)
      ldap_action_obj.delete_account(self.account_cleanup_dict)

      # Erase haproxy configurations
      self.cleanup_haproxy_configurations()

      # cleanup ldap config and schemas
      self.delete_ldap_config()

      # Delete topic created for background delete
      bgdeleteconfig = CORTXS3Config()
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        sys.stdout.write('INFO: Deleting topic.\n')
        self.delete_topic(bgdeleteconfig.get_msgbus_admin_id, bgdeleteconfig.get_msgbus_topic())
        sys.stdout.write('INFO:Topic deletion successful.\n')

      # revert config files to their origional config state
      sys.stdout.write('INFO: Reverting config files.\n')
      self.revert_config_files()
      sys.stdout.write('INFO: Reverting config files successful.\n')

      try:
        sys.stdout.write("Stopping slapd service...\n")
        service_list = ["slapd"]
        self.shutdown_services(service_list)
      except Exception as e:
        sys.stderr.write(f'Failed to stop slapd service, error: {e}\n')
        raise e
      sys.stdout.write("Stopped slapd service...\n")

      # cleanup ldap config and schemas
      self.delete_ldap_config()

      # delete slapd logs
      slapd_log="/var/log/slapd.log"
      if os.path.isfile(slapd_log):
        os.remove(slapd_log)
        sys.stdout.write(f"{slapd_log} removed\n")

    except Exception as e:
      raise e

  def revert_config_files(self):
    """Revert config files to their original config state."""

    configFiles = ["/opt/seagate/cortx/auth/resources/authserver.properties",
                  "/opt/seagate/cortx/auth/resources/keystore.properties",
                  "/opt/seagate/cortx/s3/conf/s3config.yaml",
                  "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml"]

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
      sys.stderr.write(f'Failed to revert config files in Cleanup phase, error: {e}\n')
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
    sys.stdout.write(f"Processing {self.name} detect_if_reset_done\n")
    # Validate log file cleanup.
    log_files = ['/var/log/seagate/auth/server/app.log', '/var/log/seagate/s3/s3server-*/s3server.INFO']
    for fpath in log_files:
      if os.path.exists(fpath):
        raise S3PROVError("Stale log files found in system!!!! hence reset needs to be performed before cleanup can be processed\n")
    # Validate ldap entry cleanup.
    self.validate_ldap_account_cleanup()
    sys.stdout.write(f"Processing {self.name} detect_if_reset_done completed successfully..\n")

  def validate_ldap_account_cleanup(self):
    """Validate ldap data is cleaned."""
    account_count=0
    try :
      sys.stdout.write("INFO:Validating ldap account entries\n")
      ldap_action_obj = LdapAccountAction(self.ldap_user, self.ldap_passwd)
      account_count = ldap_action_obj.get_account_count()
    except Exception as e:
      sys.stderr.write(f"ERROR: Failed to find total count of ldap account, error: {str(e)}\n")
      raise e
    if account_count > 1:
      raise S3PROVError("Stale account entries found in ldap !!!! hence reset needs to be performed before cleanup can be processed\n")
    sys.stdout.write("INFO:Validation of ldap account entries successful.\n")

  def delete_ldap_config(self):
    """Delete the ldap configs created by setup_ldap.sh during config phase."""
    # Clean up old configuration if any
    # Removing schemas
    files = self.ldap_configs["files"]
    files_wild = self.ldap_configs["files_wild"]
    dirs = self.ldap_configs["dirs"]

    for curr_file in files:
      if os.path.isfile(curr_file):
        os.remove(curr_file)
        sys.stdout.write(f"{curr_file} removed\n")

    for file_wild in files_wild:
      for path in Path(f"{file_wild['path']}").glob(f"{file_wild['glob']}"):
        if os.path.isfile(path):
          os.remove(path)
          sys.stdout.write(f"{path} removed\n")
        elif os.path.isdir(path):
          shutil.rmtree(path)
          sys.stdout.write(f"{path} removed\n")

    for curr_dir in dirs:
      if os.path.isdir(curr_dir):
        shutil.rmtree(curr_dir)
        sys.stdout.write(f"{curr_dir} removed\n")
    self.delete_mdb_files()
    sys.stdout.write("/var/lib/ldap removed\n")

  def delete_topic(self, admin_id, topic_name):
    """delete topic for background delete services."""
    try:
      s3MessageBus = S3CortxMsgBus()
      s3MessageBus.connect()
      if S3CortxMsgBus.is_topic_exist(admin_id, topic_name):
        S3CortxMsgBus.delete_topic(admin_id, [topic_name])
    except Exception as e:
      raise e

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

from setupcmd import SetupCmd
from ldapaccountaction import LdapAccountAction

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
    try:
      # Check if reset phase was performed before this
      self.detect_if_reset_done()

      # cleanup ldap accounts related to S3
      ldap_action_obj = LdapAccountAction(self.ldap_user, self.ldap_passwd)
      ldap_action_obj.delete_account(self.account_cleanup_dict)

      # revert config files to their origional config state
      self.revert_config_files()

      # cleanup ldap config and schemas
      self.delete_ldap_config()

      # TODO: Erase haproxy configurations (ref: PR #769)
    except Exception as e:
      raise e

  def revert_config_files(self):
    """Revert config files to their origional config state."""
    auth_config="/opt/seagate/cortx/auth/resources/authserver.properties"
    auth_keystore_config="/opt/seagate/cortx/auth/resources/keystore.properties"
    auth_jksstore="/opt/seagate/cortx/auth/resources/s3authserver.jks"
    auth_jksstore_template="/opt/seagate/cortx/auth/scripts/s3authserver.jks_template"
    s3_config="/opt/seagate/cortx/s3/conf/s3config.yaml"
    s3_bgdelete_config="/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml"

    try:
      if os.path.isfile(auth_config):
        shutil.copy(auth_config+".sample", auth_config)
      if os.path.isfile(auth_keystore_config):
        shutil.copy(auth_keystore_config+".sample", auth_keystore_config)
      if os.path.isfile(auth_jksstore):
        shutil.copy(auth_jksstore_template, auth_jksstore)
      if os.path.isfile(s3_config):
        shutil.copy(s3_config+".sample", s3_config)
      if os.path.isfile(s3_bgdelete_config):
        shutil.copy(s3_bgdelete_config+".sample", s3_bgdelete_config)
    except Exception as e:
      sys.stderr.write(f'Failed to revert config files in Cleanup phase, error: {e}\n')
      raise e

  def detect_if_reset_done(self):
    """TODO: Implement this handler, if reset not done, throw exception."""
    sys.stdout.write(f"Processing {self.name} detect_if_reset_done, TODO: implement it\n")
    # if reset_not_done
        # raise S3PROVError("reset needs to be performed before cleanup can be processed\n")

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

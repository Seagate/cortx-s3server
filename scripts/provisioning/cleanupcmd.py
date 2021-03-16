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
      ldap_action_obj = LdapAccountAction(self.ldap_user, self.ldap_passwd)
      ldap_action_obj.delete_account(self.account_cleanup_dict)
      self.cleanup_haproxy_configurations()
    except Exception as e:
      raise e

    try:
      self.revert_config_files()
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


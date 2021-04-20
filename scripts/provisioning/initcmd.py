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

from setupcmd import SetupCmd
from ldapaccountaction import LdapAccountAction

class InitCmd(SetupCmd):
  """Init Setup Cmd."""
  name = "init"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(InitCmd, self).__init__(config)
      self.read_ldap_credentials()
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    try:
      # Create background delete account
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

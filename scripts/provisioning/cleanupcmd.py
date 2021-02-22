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

from s3cortxutils.s3confstore.s3confstore.cortx_s3_confstore import S3CortxConfStore
from s3cortxutils.s3cipher.s3cipher.cortx_s3_cipher import CortxS3Cipher
from scripts.provisioning.setupcmd import SetupCmd

class CleanupCmd(SetupCmd):
  """Cleanup Setup Cmd."""
  name = "cleanup"
  # map with key as 'account-name', and value as the account related constants
  account_cleanup_dict = {
                          "s3-background-delete-svc": {
                            "cipherConstKey": "s3backgroundaccesskey",
                            "s3userId": "450"
                          }
                        }
  ldap_conn = None
  ldap_url = None
  ldap_cn = None

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(CleanupCmd, self).__init__(config)

      self.read_ldap_credentials()

      localconfstore = S3CortxConfStore(f'yaml://{self.s3_prov_config}', 'localindex')
      self.ldap_url = localconfstore.get_config('LDAP_URL')
      self.ldap_cn = localconfstore.get_config('LDAP_USER')

    except Exception as e:
      sys.stderr.write(f'Failed to read ldap credentials, error: {e}\n')
      raise e

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")

    # remove accounts in account_cleanup_list
    # 1. Check if the account exists or not
    # 2. Delete the account if it exists, else do nothing
    self.create_ldap_connection()

    for acc in self.account_cleanup_dict.keys():
      if(self.if_ldap_account_exists(acc)):
        sys.stdout.write(f'deleting ldap account: {acc}\n')
        try:
          self.delete_ldap_account(acc)
        except Exception as e:
          raise e
      else:
        sys.stdout.write(f'INFO: ldap account: {acc} does not exist, nothing to do.\n')

    # close ldap connection
    self.delete_ldap_connection()

  def create_ldap_connection(self):
    """Open ldap connection."""
    from ldap import initialize
    from ldap import VERSION3
    from ldap import OPT_REFERRALS

    self.ldap_conn = initialize(self.ldap_url)

    self.ldap_conn.protocol_version = VERSION3
    self.ldap_conn.set_option(OPT_REFERRALS, 0)
    self.ldap_conn.simple_bind_s(self.ldap_cn.format(self.ldap_user), self.ldap_passwd)

  def delete_ldap_connection(self):
    """Delete ldap connection."""
    self.ldap_conn.unbind_s()

  def if_ldap_account_exists(self, ldap_acc: str):
    """Check if given s3 account exists."""
    from ldap import SCOPE_SUBTREE
    from ldap import NO_SUCH_OBJECT
    try:
      self.ldap_conn.search_s(f'o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com',
                            SCOPE_SUBTREE)
    except NO_SUCH_OBJECT:
      return False
    except Exception as e:
      sys.stderr.write(f'INFO: Failed to find ldap account: {ldap_acc}, error: {str(e)}\n')
      raise e
    return True

  def delete_ldap_account(self, ldap_acc: str):
    """Delete the given s3 account."""
    try:
      acc_attributes_dict = self.account_cleanup_dict[ldap_acc]
      s3cipher_obj = CortxS3Cipher(None, True, 22, acc_attributes_dict["cipherConstKey"])
      access_key = s3cipher_obj.generate_key()
      # delete the access key
      from ldap import NO_SUCH_OBJECT
      try:
        self.ldap_conn.delete_s(f'ak={access_key},ou=accesskeys,dc=s3,dc=seagate,dc=com')
      except NO_SUCH_OBJECT:
        pass

      # delete the 'ldap_acc' account's subordinate objects
      try:
        self.ldap_conn.delete_s(f'ou=roles,o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com')
      except NO_SUCH_OBJECT:
        pass
      try:
        self.ldap_conn.delete_s(f's3UserId={acc_attributes_dict["s3userId"]},ou=users,o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com')
      except NO_SUCH_OBJECT:
        pass
      try:
        self.ldap_conn.delete_s(f'ou=users,o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com')
      except NO_SUCH_OBJECT:
        pass
      try:
        self.ldap_conn.delete_s(f'ou=groups,o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com')
      except NO_SUCH_OBJECT:
        pass
      try:
        self.ldap_conn.delete_s(f'ou=policies,o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com')
      except NO_SUCH_OBJECT:
        pass

      # delete the 'ldap_acc' account
      self.ldap_conn.delete_s(f'o={ldap_acc},ou=accounts,dc=s3,dc=seagate,dc=com')
    except Exception as e:
      sys.stderr.write(f'failed to delete account: {ldap_acc}, err: {e}\n')
      raise e

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

import ldap
import sys
import ldap.modlist as modlist
from s3cipher.cortx_s3_cipher import CortxS3Cipher

LDAP_USER = "cn={},dc=seagate,dc=com"
LDAP_URL = "ldapi:///"

# supported ldap action and input params dict
g_supported_ldap_action_table = {
  'CreateBGDeleteAccount': {
  '--account_name': "s3-background-delete-svc",
  '--account_id': "67891",
  '--canonical_id': "C67891",
  '--mail': "s3-background-delete-svc@seagate.com",
  '--s3_user_id': "450",
  '--const_cipher_secret_str': "s3backgroundsecretkey",
  '--const_cipher_access_str': "s3backgroundaccesskey"
  }
}

# map of dn names required for account creation.
g_dn_names = {
    'account' : "o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'users' : "ou=users,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'roles' : "ou=roles,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    's3userid' : "s3userid={},ou=users,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'access_key' : "ak={},ou=accesskeys,dc=s3,dc=seagate,dc=com",
    'groups' : "ou=groups,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'policies' : "ou=policies,o={},ou=accounts,dc=s3,dc=seagate,dc=com"
}

# map of default attr required for account creation.
g_attr = {
    'account' : {'objectclass' : [b'Account']},
    'users' : { 'ou' : [b'users'], 'objectclass' : [b'organizationalunit'] },
    'roles' : { 'ou' : [b'roles'], 'objectclass' : [b'organizationalunit'] },
    's3userid' : { 'path' : [b'/'], 'cn' : [b'root'], 'objectclass' :  [b'iamUser']},
    'access_key' : { 'status' : [b'Active'], 'objectclass' : [b'accessKey']},
    'groups' : { 'ou' : [b'groups'], 'objectclass' : [b'organizationalunit'] },
    'policies' : { 'ou' : [b'policies'], 'objectclass' : [b'organizationalunit'] }
}

class LdapAccountAction:
  """Perform given supported LDAP account action."""
  ldapuser = None
  ldappasswd = None
  ldap_account_action = None

  def __init__(self, ldapuser: str, ldappasswd: str, account_action: str):
    """Constructor."""
    try:
      self.ldapuser = ldapuser.strip()
      self.ldappasswd = ldappasswd.strip()

      if account_action not in g_supported_ldap_action_table.keys():
        raise Exception(f'Unsupported ldap account action: {account_action}')

      self.ldap_account_action = account_action.strip()
    except Exception as e:
      sys.stderr.write(f'initialization failed, err: {e}')
      raise e

  def __add_keys_to_dictionary(self, input_params:dict):
    """Adds access and secret keys to dictionary."""
    access_key, secret_key = self.__generate_access_secret_keys(
                                      input_params['--const_cipher_secret_str'],
                                      input_params['--const_cipher_access_str'])
        
    input_params['--access_key'] = access_key
    input_params['--secret_key'] = secret_key

  @staticmethod
  def __get_attr(index_key):
    """Fetches attr from map based on index."""
    if index_key in g_attr :
      return g_attr[index_key]

    raise Exception("Key Not Present")

  @staticmethod
  def __get_dn(index_key):
    """Fetches dn string from map based on index."""
    if index_key in g_dn_names:
      return g_dn_names[index_key]

    raise Exception("Key Not Present")

  def __create_account_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for creating 'account'."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)

    attrs['o'] = [input_params['--account_name'].encode('utf-8')]
    attrs['accountid'] = [input_params['--account_id'].encode('utf-8')]
    attrs['mail'] = [input_params['--mail'].encode('utf-8')]
    attrs['canonicalId'] = [input_params['--canonical_id'].encode('utf-8')]

    dn = dn.format(input_params['--account_name'])

    return dn, attrs

  def __create_default_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for generic case."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)
    dn = dn.format(input_params['--account_name'])
    return dn, attrs

  def __create_s3userid_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for creating 's3userid'."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)

    arn = "arn:aws:iam::{}:root".format(input_params['--account_id'])

    attrs['s3userid'] = [input_params['--s3_user_id'].encode('utf-8')]
    attrs['arn'] = [arn.encode('utf-8')]

    dn = dn.format(input_params['--s3_user_id'], input_params['--account_name'])
    return dn, attrs

  def __create_accesskey_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for creating 'accesskey'."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)

    attrs['ak'] = [input_params['--access_key'].encode('utf-8')]
    attrs['s3userid'] = [input_params['--s3_user_id'].encode('utf-8')]
    attrs['sk'] = [input_params['--secret_key'].encode('utf-8')]

    dn = dn.format(input_params['--access_key'])
    return dn, attrs

  @staticmethod
  def __generate_access_secret_keys(const_secret_string, const_access_string):
    """Generates access and secret keys."""
    cortx_access_key = CortxS3Cipher(None, True, 22, const_access_string).generate_key()
    cortx_secret_key = CortxS3Cipher(None, False, 40, const_secret_string).generate_key()
    return cortx_access_key, cortx_secret_key

  @staticmethod
  def __connect_to_ldap_server(ldapuser, ldappasswd):
    """Establish connection to ldap server."""
    ldap_connection = ldap.initialize(LDAP_URL)
    ldap_connection.protocol_version = ldap.VERSION3
    ldap_connection.set_option(ldap.OPT_REFERRALS, 0)
    ldap_connection.simple_bind_s(LDAP_USER.format(ldapuser), ldappasswd)
    return ldap_connection

  @staticmethod
  def __is_account_present(account_name, ldap_connection):
    """Checks if account is present in ldap db."""
    try:
      ldap_connection.search_s(f"o={account_name},ou=accounts,dc=s3,dc=seagate,dc=com", ldap.SCOPE_SUBTREE)
    except ldap.NO_SUCH_OBJECT:
      return False
    except Exception as e:
      sys.stderr.write(f'INFO: Failed to find ldap account: {account_name}, error: {str(e)}\n')
      raise e
    return True

  @staticmethod
  def __disconnect_from_ldap(ldap_connection):
    """Disconnects from ldap"""
    ldap_connection.unbind_s()

  def create_account(self):
    """Creates account in ldap db."""
    ldap_connection = None
    input_params = g_supported_ldap_action_table[self.ldap_account_action]
    try:
      ldap_connection = self.__connect_to_ldap_server(self.ldapuser,
                                              self.ldappasswd)
      self.__add_keys_to_dictionary(input_params)

      if not self.__is_account_present(input_params['--account_name'], ldap_connection):
        # create the account
        dn, attrs = self.__create_account_prepare_params('account', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)

        # create the account's sub-ordinate objects
        dn, attrs = self.__create_default_prepare_params('policies', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)

        dn, attrs = self.__create_default_prepare_params('groups', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)

        dn, attrs = self.__create_default_prepare_params('users', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)

        dn, attrs = self.__create_s3userid_prepare_params('s3userid', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)

        dn, attrs = self.__create_default_prepare_params('roles', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)

        dn, attrs = self.__create_accesskey_prepare_params('access_key', input_params)
        ldif = modlist.addModlist(attrs)
        ldap_connection.add_s(dn, ldif)
      else:
        sys.stdout.write(f"ldap account: {input_params['--account_name']} already exists.\n")

      self.__disconnect_from_ldap(ldap_connection)
    except Exception as e:
      if ldap_connection:
        self.__disconnect_from_ldap(ldap_connection)
      raise e

  @staticmethod
  def supported_ldap_actions():
    """Get supported actions."""
    print(f'supported ldap actions: {list(g_supported_ldap_action_table.keys())}')

  @staticmethod
  def print_create_account_results(result:dict):
    """Prints results of create account action."""
    print(f"AccountId = {result['--account_id']}, CanonicalId = {result['--canonical_id']},"
        f" RootUserName = root, AccessKeyId = {result['--access_key']}, SecretKey = {result['--secret_key']}")

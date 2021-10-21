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
import socket
from ldap.ldapobject import SimpleLDAPObject
import ldap.modlist as modlist
from s3cipher.cortx_s3_cipher import CortxS3Cipher
from subprocess import check_output
import logging

LDAP_USER = "cn={},dc=seagate,dc=com"
LDAP_URL = "ldapi:///"

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
  ldap_conn = None

  def __init__(self, ldapuser: str, ldappasswd: str):
    """Constructor."""
    s3deployment_logger_name = "s3-deployment-logger-" + "[" + str(socket.gethostname()) + "]"
    self.logger = logging.getLogger(s3deployment_logger_name)
    if self.logger.hasHandlers():
      self.logger.info("Logger has valid handler")
    else:
      self.logger.setLevel(logging.DEBUG)
      # create console handler with a higher log level
      chandler = logging.StreamHandler(sys.stdout)
      chandler.setLevel(logging.DEBUG)
      s3deployment_log_format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
      formatter = logging.Formatter(s3deployment_log_format)
      # create formatter and add it to the handlers
      chandler.setFormatter(formatter)
      # add the handlers to the logger
      self.logger.addHandler(chandler)

    try:
      self.ldapuser = ldapuser.strip()
      self.ldappasswd = ldappasswd.strip()

    except Exception as e:
      self.logger.error(f'initialization failed, err: {e}')
      raise e

  def __add_keys_to_dictionary(self, input_params:dict):
    """Adds access and secret keys to dictionary."""
    access_key, secret_key = self.__generate_access_secret_keys(
                                      input_params['const_cipher_secret_str'],
                                      input_params['const_cipher_access_str'])

    encrypted_secret_key = self.__encrypt_secret_key(secret_key)

    input_params['access_key'] = access_key
    input_params['secret_key'] = secret_key
    input_params['encrypted_secret_key'] = encrypted_secret_key

  def __create_account_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for creating 'account'."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)

    attrs['o'] = [input_params['account_name'].encode('utf-8')]
    attrs['accountid'] = [input_params['account_id'].encode('utf-8')]
    attrs['mail'] = [input_params['mail'].encode('utf-8')]
    attrs['canonicalId'] = [input_params['canonical_id'].encode('utf-8')]

    dn = dn.format(input_params['account_name'])

    return dn, attrs

  def __create_default_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for generic case."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)
    dn = dn.format(input_params['account_name'])
    return dn, attrs

  def __create_s3userid_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for creating 's3userid'."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)

    arn = "arn:aws:iam::{}:root".format(input_params['account_id'])

    attrs['s3userid'] = [input_params['s3_user_id'].encode('utf-8')]
    attrs['arn'] = [arn.encode('utf-8')]

    dn = dn.format(input_params['s3_user_id'], input_params['account_name'])
    return dn, attrs

  def __create_accesskey_prepare_params(self, index_key:str, input_params:dict):
    """Builds params for creating 'accesskey'."""
    dn = self.__get_dn(index_key)
    attrs = self.__get_attr(index_key)

    attrs['ak'] = [input_params['access_key'].encode('utf-8')]
    attrs['s3userid'] = [input_params['s3_user_id'].encode('utf-8')]
    attrs['sk'] = [input_params['encrypted_secret_key'].encode('utf-8')]

    dn = dn.format(input_params['access_key'])
    return dn, attrs

  def __connect_to_ldap_server(self):
    """Establish connection to ldap server."""
    from ldap import initialize
    from ldap import VERSION3
    from ldap import OPT_REFERRALS

    self.ldap_conn = initialize(LDAP_URL)
    self.ldap_conn.protocol_version = VERSION3
    self.ldap_conn.set_option(OPT_REFERRALS, 0)
    self.ldap_conn.simple_bind_s(LDAP_USER.format(self.ldapuser), self.ldappasswd)

  def __disconnect_from_ldap(self):
    """Disconnects from ldap."""
    self.ldap_conn.unbind_s()

  def create_account(self, input_params: dict) -> None:
    """Creates account in ldap db."""
    self.__add_keys_to_dictionary(input_params)
    try:
      self.__connect_to_ldap_server()

      if not self.__is_account_present(self.ldap_conn, input_params['account_name']):
        # create the account
        dn, attrs = self.__create_account_prepare_params('account', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)

        # create the account's sub-ordinate objects
        dn, attrs = self.__create_default_prepare_params('policies', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)

        dn, attrs = self.__create_default_prepare_params('groups', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)

        dn, attrs = self.__create_default_prepare_params('users', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)

        dn, attrs = self.__create_s3userid_prepare_params('s3userid', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)

        dn, attrs = self.__create_default_prepare_params('roles', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)

        dn, attrs = self.__create_accesskey_prepare_params('access_key', input_params)
        ldif = modlist.addModlist(attrs)
        self.ldap_conn.add_s(dn, ldif)
      else:
        self.logger.info(f"ldap account: {input_params['account_name']} already exists.")

      self.__disconnect_from_ldap()
    except Exception as e:
      if self.ldap_conn:
        self.__disconnect_from_ldap()
      raise e


  def delete_account(self, input_params: dict) -> None:
    """
    Delete ldap account with given input params.
    @input_params: dictionary of format:
    {'account_name': {'s3userId': 'user_id' } }
    """
    try:
      self.__connect_to_ldap_server()

      for acc in input_params.keys():
        if not self.__is_account_present(self.ldap_conn, acc):
          self.logger.info(f'LDAP account: {acc} does not exist, skipping deletion.')
          continue
        acc_attr_dict = input_params[acc]

        # delete the access key
        self.__delete_access_key(self.ldap_conn, acc_attr_dict['s3userId'])

        # delete roles
        self.__delete_dn(self.ldap_conn, f'ou=roles,o={acc},ou=accounts,dc=s3,dc=seagate,dc=com')

        # delete s3userid
        self.__delete_dn(self.ldap_conn,
                    f"s3UserId={acc_attr_dict['s3userId']},ou=users,o={acc},ou=accounts,dc=s3,dc=seagate,dc=com")

        # delete users
        self.__delete_dn(self.ldap_conn, f'ou=users,o={acc},ou=accounts,dc=s3,dc=seagate,dc=com')

        # delete groups
        self.__delete_dn(self.ldap_conn, f'ou=groups,o={acc},ou=accounts,dc=s3,dc=seagate,dc=com')

        # delete policies
        self.__delete_dn(self.ldap_conn, f'ou=policies,o={acc},ou=accounts,dc=s3,dc=seagate,dc=com')

        # delete the account
        self.__delete_dn(self.ldap_conn, f'o={acc},ou=accounts,dc=s3,dc=seagate,dc=com')

      self.__disconnect_from_ldap()
    except Exception as e:
      self.logger.error(f'Failed to delete account: {acc}')
      raise e

  def get_account_count(self) -> None:
    """Get total count of account in ldap db."""
    try:
      self.__connect_to_ldap_server()
      result_list = self.ldap_conn.search_s("ou=accounts,dc=s3,dc=seagate,dc=com", ldap.SCOPE_SUBTREE,
       filterstr='(ObjectClass=Account)')
      self.__disconnect_from_ldap()
      return len(result_list)

    except ldap.NO_SUCH_OBJECT:
      if self.ldap_conn:
        self.__disconnect_from_ldap()
      return 0
    except Exception as e:
      if self.ldap_conn:
        self.__disconnect_from_ldap()
      self.logger.error(f'ERROR: Failed to get count of ldap account, error: {str(e)}')
      raise e

  def delete_s3_ldap_data(self):
    """Delete all s3 data entries from ldap."""
    cleanup_records = ["ou=accesskeys,dc=s3,dc=seagate,dc=com",
                        "ou=accounts,dc=s3,dc=seagate,dc=com",
                        "ou=idp,dc=s3,dc=seagate,dc=com"]
    try:
      self.logger.info('Deletion of ldap data started.')
      self.__connect_to_ldap_server()
      for entry in cleanup_records:
        self.logger.info(' deleting all entries from {entry} & its sub-ordinate tree')
        try:
          self.ldap_delete_recursive(self.ldap_conn, entry)
        except ldap.NO_SUCH_OBJECT:
          # If no entries found in ldap for given dn
          pass
      self.__disconnect_from_ldap()
      self.logger.info('Deletion of ldap data completed successfully.')
    except Exception as e:
      if self.ldap_conn:
        self.__disconnect_from_ldap()
      self.logger.error(f'ERROR: Failed to delete ldap data, error: {str(e)}')
      raise e

  def ldap_delete_recursive(self, ldap_conn: SimpleLDAPObject, base_dn: str):
    """Delete all objects and its subordinate entries of given base_dn from ldap."""
    l_search = ldap_conn.search_s(base_dn, ldap.SCOPE_ONELEVEL)
    for dn, _ in l_search:
        if not dn == base_dn:
          self.ldap_delete_recursive(self.ldap_conn, dn)
          ldap_conn.delete_s(dn)

  def __is_account_present(self, ldap_conn: SimpleLDAPObject, account_name: str):
    """Checks if account is present in ldap db."""
    try:
      ldap_conn.search_s(f"o={account_name},ou=accounts,dc=s3,dc=seagate,dc=com", ldap.SCOPE_SUBTREE)
    except ldap.NO_SUCH_OBJECT:
      return False
    except Exception as e:
      self.logger.error(f'ERROR: Failed to find ldap account: {account_name}, error: {str(e)}')
      raise e
    return True

  def __delete_dn(self, ldap_conn: SimpleLDAPObject, dn: str):
    """Delete given DN from ldap."""
    try:
      ldap_conn.delete_s(dn)
    except ldap.NO_SUCH_OBJECT:
      pass
    except Exception as e:
      self.logger.error(f'Failed to delete DN: {dn}\n')
      raise e

  def __delete_access_key(self, ldap_conn: SimpleLDAPObject, userid: str):
    """Delete access key of given s3userid."""
    try:
      access_key = self.__get_accesskey(ldap_conn, userid)
      ldap_conn.delete_s(f'ak={access_key},ou=accesskeys,dc=s3,dc=seagate,dc=com')
    except ldap.NO_SUCH_OBJECT:
      pass
    except Exception as e:
      self.logger.error(f'failed to delete access key of userid: {userid}')
      raise e

  def __get_accesskey(self, ldap_conn: SimpleLDAPObject, s3userid: str) -> str:
    """Get accesskey of the given userid."""
    access_key = None

    from ldap import SCOPE_SUBTREE

    result_list = ldap_conn.search_s('ou=accesskeys,dc=s3,dc=seagate,dc=com',
                                    SCOPE_SUBTREE,
                                    filterstr='(ObjectClass=accessKey)')
    for (_, attr_dict) in result_list:
      if s3userid == attr_dict['s3UserId'][0].decode():
        access_key = attr_dict['ak'][0].decode()
        break
    return access_key

  def __generate_access_secret_keys(self, const_secret_string, const_access_string):
    """Generates access and secret keys."""
    cortx_access_key = CortxS3Cipher(None, True, 22, const_access_string).generate_key()
    cortx_secret_key = CortxS3Cipher(None, False, 40, const_secret_string).generate_key()
    return cortx_access_key, cortx_secret_key

  def __encrypt_secret_key(self, secret_key):
    return check_output(['java', '-jar', '/opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar', '-s', secret_key, '-e', 'aes']).rstrip().decode()


  def __get_attr(self, index_key):
    """Fetches attr from map based on index."""
    if index_key in g_attr :
      return g_attr[index_key]

    raise Exception("Key Not Present")

  def __get_dn(self, index_key):
    """Fetches dn string from map based on index."""
    if index_key in g_dn_names:
      return g_dn_names[index_key]

    raise Exception("Key Not Present")

  @staticmethod
  def print_create_account_results(result:dict) -> None:
    """Prints results of create account action."""
    print(f"AccountId = {result['account_id']}, CanonicalId = {result['canonical_id']},"
        f" RootUserName = root, AccessKeyId = {result['access_key']}, SecretKey = {result['secret_key']}")

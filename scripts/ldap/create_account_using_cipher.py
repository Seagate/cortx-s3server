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
import traceback
from s3cipher.cortx_s3_cipher import CortxS3Cipher

"""
Constants
"""
LDAP_USER = "cn={},dc=seagate,dc=com"
LDAP_URL = "ldapi:///"

"""
Global variables
"""

#map of dn names required for account creation.
g_dn_names = {
    'account' : "o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'users' : "ou=users,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'roles' : "ou=roles,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    's3userid' : "s3userid={},ou=users,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'access_key' : "ak={},ou=accesskeys,dc=s3,dc=seagate,dc=com",
    'groups' : "ou=groups,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'policies' : "ou=policies,o={},ou=accounts,dc=s3,dc=seagate,dc=com"
}

#map of default attr required for account creation.
g_attr = {
    'account' : {'objectclass' : [b'Account']},
    'users' : { 'ou' : [b'users'], 'objectclass' : [b'organizationalunit'] },
    'roles' : { 'ou' : [b'roles'], 'objectclass' : [b'organizationalunit'] },
    's3userid' : { 'path' : [b'/'], 'cn' : [b'root'], 'objectclass' :  [b'iamUser']},
    'access_key' : { 'status' : [b'Active'], 'objectclass' : [b'accessKey']},
    'groups' : { 'ou' : [b'groups'], 'objectclass' : [b'organizationalunit'] },
    'policies' : { 'ou' : [b'policies'], 'objectclass' : [b'organizationalunit'] }
}

def add_keys_to_dictionary(input_params:dict):
    """
    Adds access and secret keys to dictionary.
    """
    access_key, secret_key = generate_access_secret_keys(
        input_params['--const_cipher_secret_str'],
        input_params['--const_cipher_access_str'])
    
    input_params['--secret_key'] = secret_key
    input_params['--access_key'] = access_key

def get_attr(index_key):
    """
    Fetches attr from map based on index.
    """
    if index_key in g_attr :
        return g_attr[index_key]
    
    raise Exception("Key Not Present")

def get_dn(index_key):
    """
    Fetches dn string from map based on index.
    """
    if index_key in g_dn_names:
        return g_dn_names[index_key]

    raise Exception("Key Not Present")

def create_account_prepare_params(index_key:str, input_params:dict):
    """
    Builds params for creating 'account'.
    """
    dn = get_dn(index_key)
    attrs = get_attr(index_key)

    attrs['o'] = [input_params['--account_name'].encode('utf-8')]
    attrs['accountid'] = [input_params['--account_id'].encode('utf-8')]
    attrs['mail'] = [input_params['--mail'].encode('utf-8')]
    attrs['canonicalId'] = [input_params['--canonical_id'].encode('utf-8')]

    dn = dn.format(input_params['--account_name'])

    return dn, attrs

def create_default_prepare_params(index_key:str, input_params:dict):
    """
    Builds params for generic case.
    """
    dn = get_dn(index_key)
    attrs = get_attr(index_key)
    dn = dn.format(input_params['--account_name'])
    return dn, attrs

def create_s3userid_prepare_params(index_key:str, input_params:dict):
    """
    Builds params for creating 's3userid'.
    """    
    dn = get_dn(index_key)
    attrs = get_attr(index_key)

    arn = "arn:aws:iam::{}:root".format(input_params['--account_id'])

    attrs['s3userid'] = [input_params['--s3_user_id'].encode('utf-8')]
    attrs['arn'] = [arn.encode('utf-8')]

    dn = dn.format(input_params['--s3_user_id'], input_params['--account_name'])
    return dn, attrs

def create_accesskey_prepare_params(index_key:str, input_params:dict):
    """
    Builds params for creating 'accesskey'.
    """    
    dn = get_dn(index_key)
    attrs = get_attr(index_key)

    attrs['ak'] = [input_params['--access_key'].encode('utf-8')]
    attrs['s3userid'] = [input_params['--s3_user_id'].encode('utf-8')]
    attrs['sk'] = [input_params['--secret_key'].encode('utf-8')]    

    dn = dn.format(input_params['--access_key'])
    return dn, attrs

#map of functions to generalize account creation.
g_create_func_table = [
    {'key' : 'account', 'func' : create_account_prepare_params},
    {'key' : 'users', 'func' : create_default_prepare_params},
    {'key' : 'roles', 'func' : create_default_prepare_params},
    {'key' : 's3userid', 'func' : create_s3userid_prepare_params},
    {'key' : 'access_key', 'func' : create_accesskey_prepare_params},
    {'key' : 'groups', 'func' : create_default_prepare_params},
    {'key' : 'policies', 'func' : create_default_prepare_params}
]

def generate_key(config, use_base64, key_len, const_key):
    """
    Generates a key based on input parameters.
    """
    s3cipher = CortxS3Cipher(config, use_base64, key_len, const_key)
    return s3cipher.generate_key()

def generate_access_secret_keys(const_secret_string, const_access_string):
    """
    Generates access and secret keys.
    """
    cortx_access_key = generate_key(None, True, 22, const_access_string)
    cortx_secret_key = generate_key(None, False, 40, const_secret_string)
    return cortx_access_key, cortx_secret_key

def connect_to_ldap_server(ldapuser, ldappasswd):
    """
    Establish connection to ldap server.
    """
    ldap_connection = ldap.initialize(LDAP_URL)
    ldap_connection.protocol_version = ldap.VERSION3
    ldap_connection.set_option(ldap.OPT_REFERRALS, 0)
    ldap_connection.simple_bind_s(LDAP_USER.format(ldapuser), ldappasswd)
    return ldap_connection

def is_account_present(account_name, ldap_connection):
    """
    Checks if account is present in ldap db.
    """
    try:
        results = ldap_connection.search_s("o={},ou=accounts,dc=s3,dc=seagate,dc=com".format(account_name), ldap.SCOPE_SUBTREE)
    except Exception:
        return False

    if len(results) < 6:
        raise Exception("{} account creation was corrupted, use s3iamcli to DeleteAccount".format(account_name))
    else:
        return True

def disconnect_from_ldap(ldap_connection):
    """
    Disconnects from ldap
    """
    ldap_connection.unbind_s()

def create_account(input_params:dict):
    """
    Creates account in ldap db.
    """
    ldap_connection = None
    try:
        ldap_connection = connect_to_ldap_server(input_params['--ldapuser'],
            input_params['--ldappasswd'])
        add_keys_to_dictionary(input_params)

        if not is_account_present(input_params['--account_name'], ldap_connection):
            for item in g_create_func_table:
                dn, attrs = item['func'](item['key'],input_params)
                ldif = modlist.addModlist(attrs)
                ldap_connection.add_s(dn,ldif)

        disconnect_from_ldap(ldap_connection)
    except Exception as e:
        if ldap_connection:
            disconnect_from_ldap(ldap_connection)
        raise e

"""
Master Script Stuff
"""

#parameter map for bgdelete
g_bgdelete_create_account_input_params = {
    '--account_name' : "s3-background-delete-svc",
    '--account_id' : "67891",
    '--canonical_id' : "C67891",
    '--mail' : "s3-background-delete-svc@seagate.com",
    '--s3_user_id' : "450",
    '--const_cipher_secret_str' : "s3backgroundsecretkey",
    '--const_cipher_access_str' : "s3backgroundaccesskey"
}

#parameter map for recovery
g_recovery_create_account_input_params = {
    '--account_name' : "s3-recovery-svc",
    '--account_id' : "67892",
    '--canonical_id' : "C67892",
    '--mail' : "s3-recovery-svc@seagate.com",
    '--s3_user_id' : "451",
    '--const_cipher_secret_str' : "s3recoverysecretkey",
    '--const_cipher_access_str' : "s3recoveryaccesskey"
}

#input parameter checklist for script.(creating account only)
g_create_input_params_list = ['--ldapuser', '--ldappasswd']

def print_script_usage(args:list = None):
    """
    Prints help for the script.
    """
    print("[Usage:]\ncreate_s3background_account_cipher.py CreateBGDeleteAccount/CreateRecoveryAccount --ldapuser {username} --ldappasswd {passwd}")

def print_create_account_results(result:dict):
    """
    Prints results of create account action.
    """
    print("AccountId = {}, CanonicalId = {}, RootUserName = root, AccessKeyId = {}, SecretKey = {}".
        format(result['--account_id'], result['--canonical_id'], result['--access_key'], result['--secret_key']))

#Commandline input routing table.
g_cmdline_param_table = {
    'CreateBGDeleteAccount' : [g_bgdelete_create_account_input_params, g_create_input_params_list, create_account, print_create_account_results],
    'CreateRecoveryAccount' : [g_recovery_create_account_input_params, g_create_input_params_list, create_account, print_create_account_results]
}

def process_cmdline_args(args:list):
    """
    Take action as per cmd args
    """
    if len(args) < 1:
        print_script_usage()
        return

    try:
        if args[0] in g_cmdline_param_table:
            input_params = g_cmdline_param_table[args[0]][0]
            key = None
            for argument in args[1:]:
                if argument in g_cmdline_param_table[args[0]][1]:
                    key=argument
                elif key:
                    input_params[key] = argument
                    key = None
                else:
                    raise Exception("Invalid Parameter Passed. Check usage")
            #Actual Processing function.
            g_cmdline_param_table[args[0]][2](input_params)
            #Print Output.
            g_cmdline_param_table[args[0]][3](input_params)
        else:
            raise Exception("Invalid Parameter Passed. Check usage")
    except Exception as e:
        print("Exception : {}".format(e))
        print("Traceback : {}".format(traceback.format_exc()))
        print_script_usage()

if __name__ == "__main__":
    process_cmdline_args(sys.argv[1:])

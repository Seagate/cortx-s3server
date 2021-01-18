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

#!/usr/bin/env python3

from shutil import register_unpack_format
import ldap
import sys
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
import ldap.modlist as modlist
import traceback
from s3backgrounddelete.cortx_cluster_config import CipherInvalidToken
from s3backgrounddelete.cortx_s3_cipher import CortxS3Cipher

LDAP_USER = "cn=sgiamadmin,dc=seagate,dc=com"
LDAP_PASSWORD = "ldapadmin"
LDAP_URL = "ldapi:///"

g_default_input_params = {
    '--account_name' : "s3-background-delete-svc",
    '--account_id' : "67891",
    '--mail' : "s3-background-delete-svc@seagate.com",
    '--s3_user_id' : "450",
    '--cstr_secret' : "s3backgroundsecretkey",
    '--cstr_access' : "s3backgroundaccesskey"
}

g_dn_names = {
    'account' : "o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'users' : "ou=users,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'roles' : "ou=roles,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    's3userid' : "s3userid={},ou=users,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'access_key' : "ak={},ou=accesskeys,dc=s3,dc=seagate,dc=com",
    'groups' : "ou=groups,o={},ou=accounts,dc=s3,dc=seagate,dc=com",
    'policies' : "ou=policies,o={},ou=accounts,dc=s3,dc=seagate,dc=com"
}

g_attr = {
    'account' : {'objectclass' : [b'Account']},
    'users' : { 'ou' : [b'users'], 'objectclass' : [b'organizationalunit'] },
    'roles' : { 'ou' : [b'roles'], 'objectclass' : [b'organizationalunit'] },
    's3userid' : { 'path' : [b'/'], 'cn' : [b'root'], 'objectclass' :  [b'iamUser']},
    'access_key' : { 'status' : [b'Active'], 'objectclass' : [b'accessKey']},
    'groups' : { 'ou' : [b'groups'], 'objectclass' : [b'organizationalunit'] },
    'policies' : { 'ou' : [b'policies'], 'objectclass' : [b'organizationalunit'] }
}

def get_attr(index_key):
    if index_key in g_attr :
        return g_attr[index_key]
    
    raise Exception("Key Not Present")

def get_dn(index_key):
    if index_key in g_dn_names:
        return g_dn_names[index_key]

    raise Exception("Key Not Present")

def create_account_prepare_params(index_key:str, input_params:dict):
    dn = get_dn(index_key)
    attrs = get_attr(index_key)

    canonical_id = "C" + input_params['--account_id']

    attrs['o'] = [input_params['--account_name'].encode('utf-8')]
    attrs['accountid'] = [input_params['--account_id'].encode('utf-8')]
    attrs['mail'] = [input_params['--mail'].encode('utf-8')]
    attrs['canonicalId'] = [canonical_id.encode('utf-8')]

    dn = dn.format(input_params['--account_name'])

    return dn, attrs

def create_default_prepare_params(index_key:str, input_params:dict):
    dn = get_dn(index_key)
    attrs = get_attr(index_key)
    dn = dn.format(input_params['--account_name'])
    return dn, attrs

def create_s3userid_prepare_params(index_key:str, input_params:dict):
    dn = get_dn(index_key)
    attrs = get_attr(index_key)

    arn = "arn:aws:iam::{}:root".format(input_params['--account_id'])

    attrs['s3userid'] = [input_params['--s3_user_id'].encode('utf-8')]
    attrs['arn'] = [arn.encode('utf-8')]

    dn = dn.format(input_params['--s3_user_id'], input_params['--account_name'])
    return dn, attrs

def create_accesskey_prepare_params(index_key:str, input_params:dict):
    dn = get_dn(index_key)
    attrs = get_attr(index_key)

    attrs['ak'] = [input_params['--access_key'].encode('utf-8')]
    attrs['s3userid'] = [input_params['--s3_user_id'].encode('utf-8')]
    attrs['sk'] = [input_params['--secret_key'].encode('utf-8')]    

    dn = dn.format(input_params['--access_key'])
    return dn, attrs

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
    s3cipher = CortxS3Cipher(config, use_base64, key_len, const_key)
    return s3cipher.get_key()

def generate_access_secret_keys(const_access_string, const_secret_string):
    cortx_access_key = generate_key(None, True, 22, const_access_string)
    cortx_secret_key = generate_key(None, False, 40, const_secret_string)
    return cortx_access_key, cortx_secret_key

def connect_to_ldap_server():
    ldap_connection = ldap.initialize(LDAP_URL)
    ldap_connection.protocol_version = ldap.VERSION3
    ldap_connection.set_option(ldap.OPT_REFERRALS, 0)
    ldap_connection.simple_bind_s(LDAP_USER, LDAP_PASSWORD)
    return ldap_connection

def is_account_present(account_name, ldap_connection):
    try:
        results = ldap_connection.search_s("o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com", ldap.SCOPE_SUBTREE)
    except Exception as e:
        return False

    if len(results) < 6:
        raise Exception("s3-background-delete-svc account creation was corrupted, use s3iamcli to DeleteAccount")
    else:
        return True

def disconnect_from_ldap(ldap_connection):
    ldap_connection.unbind_s()

def generate_input_params():
    input_params = g_default_input_params

    bg_access_key, bg_secret_key = generate_access_secret_keys(
        input_params['--cstr_access'],
        input_params['--cstr_secret'])
    
    input_params['--secret_key'] = bg_secret_key
    input_params['--access_key'] = bg_access_key

    return input_params

def create_account(input_params):
    ldap_connection = None
    try:
        ldap_connection = connect_to_ldap_server()

        if not is_account_present(input_params['--account_name'], ldap_connection):

            for item in g_create_func_table:
                dn, attrs = item['func'](item['key'],input_params)
                ldif = modlist.addModlist(attrs)
                ldap_connection.add_s(dn,ldif)

            print("Account Created Successfully..")
            print("Access Key : {}, Secret Key : {}".format(input_params['--access_key'],
                input_params['--secret_key']))

        else:
            print("Account already present.")

    except Exception as e:
        print("Exception : {}".format(e))
        print("Traceback : {}".format(traceback.format_exc()))
    finally:
        if not ldap_connection:
            ldap_connection.unbind()


if __name__ == "__main__":
    input_params = generate_input_params()
    create_account(input_params)
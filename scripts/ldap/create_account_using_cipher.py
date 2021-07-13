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

import argparse
import traceback
import sys
import os

sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__),  '../../s3cortxutils/s3confstore')))
from ldapaccountaction import LdapAccountAction
from s3confstore.cortx_s3_confstore import S3CortxConfStore

# supported ldap action and input params dict
opfileconfstore = S3CortxConfStore('yaml:///opt/seagate/cortx/s3/s3backgrounddelete/config.yaml', 'read_bg_delete_config_idx')

param_list = ['account_name', 'account_id', 'canonical_id', 'mail', 's3_user_id', 'const_cipher_secret_str', 'const_cipher_access_str']
bgdelete_acc_input_params_dict = {}
for param in param_list:
    key = 'background_delete_account' + '>' + param
    value_for_key = opfileconfstore.get_config(f'{key}')
    bgdelete_acc_input_params_dict[param] = value_for_key

g_supported_ldap_action_table = {}
g_supported_ldap_action_table['CreateBGDeleteAccount'] = bgdelete_acc_input_params_dict

def supported_ldap_actions():
  """Get supported actions."""
  print(f'supported ldap actions: {list(g_supported_ldap_action_table.keys())}')

def process_cmdline_args():
  """
  Take action as per cmd args.
  """
  parser = argparse.ArgumentParser(description='create s3 account using s3cipher generated keys')
  subparsers = parser.add_subparsers(dest='action')
  create_bg_acc = subparsers.add_parser('CreateBGDeleteAccount', help='Create background delete service account')

  create_bg_acc.add_argument('--ldapuser', help='sgiam ldap user name', type=str, required=True)
  create_bg_acc.add_argument('--ldappasswd', help='sgiam ldap user password', type=str, required=True)

  args = parser.parse_args()

  try:
    if args.action in g_supported_ldap_action_table.keys():
      action_obj = LdapAccountAction(args.ldapuser, args.ldappasswd)
      if args.action == 'CreateBGDeleteAccount':
        action_obj.create_account(g_supported_ldap_action_table[args.action])

        result_dict = g_supported_ldap_action_table[args.action]
        action_obj.print_create_account_results(result_dict)
  except Exception as e:
    print("Exception : {}".format(e))
    print("Traceback : {}".format(traceback.format_exc()))
    parser.print_help()

if __name__ == "__main__":
  process_cmdline_args()

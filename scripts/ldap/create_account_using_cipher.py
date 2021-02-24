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

from ldapaccountaction import LdapAccountAction
from ldapaccountaction import g_supported_ldap_action_table

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
      action_obj = LdapAccountAction(args.ldapuser, args.ldappasswd, args.action)
      action_obj.create_account()
      result_dict = g_supported_ldap_action_table[args.action]
      action_obj.print_create_account_results(result_dict)
  except Exception as e:
    print("Exception : {}".format(e))
    print("Traceback : {}".format(traceback.format_exc()))
    parser.print_help()

if __name__ == "__main__":
  process_cmdline_args()

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

import os
import yaml
from subprocess import call

class LdapOps:
    def __init__(self):
        self.test_data_dir = os.path.join(os.path.dirname(__file__), 'test_data')
        ldap_config_file = os.path.join(self.test_data_dir, 'ldap_config.yaml')
        with open(ldap_config_file, 'r') as f:
            self.ldap_config = yaml.safe_load(f)

    def delete_account(self, account_name):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r "
                "o=%s,ou=accounts,dc=s3,dc=seagate,dc=com" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], account_name))
        call(cmd, shell=True)

    def delete_all_users(self, account_name):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \
                \"ou=users,o=%s,ou=accounts,dc=s3,dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], account_name))
        call(cmd)

    def delete_all_roles(self, account_name):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \
                \"ou=roles,o=%s,ou=accounts,dc=s3,dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], account_name))
        call(cmd)

    def delete_all_groups(self, account_name):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \
                \"ou=groups,o=%s,ou=accounts,dc=s3,dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], account_name))
        call(cmd)

    def delete_policy(self, account_name):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \
                \"ou=policies,o=%s,ou=accounts,dc=s3,dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], account_name))
        call(cmd)

    def delete_accesskey(self, access_key):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r \
                \"ak=%s,ou=accesskeys,dc=s3,dc=seagate,dc=com\"" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], access_key))
        call(cmd)

    def delete_root_user(self, account_name):
        cmd = ("ldapdelete -h %s -p %s -w %s -x -D %s -r "
                "s3UserId=*,ou=users,o=%s,ou=accounts,dc=s3,dc=seagate,dc=com" %
                (self.ldap_config['host'], self.ldap_config['port'],
                self.ldap_config['password'], self.ldap_config['login_dn'], account_name))
        print(cmd)
        call(cmd, shell=True)

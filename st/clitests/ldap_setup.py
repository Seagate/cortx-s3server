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
from subprocess import call, check_output
from shlex import split
import fileinput
import shutil

class LdapSetup:
    def __init__(self):
        self.test_data_dir = os.path.join(os.path.dirname(__file__), 'test_data')

        ldap_config_file = os.path.join(self.test_data_dir, 'ldap_config.yaml')
        with open(ldap_config_file, 'r') as f:
            self.ldap_config = yaml.safe_load(f)

    def ldap_init(self):
        ldap_init_file = os.path.join(self.test_data_dir, 'create_test_data.ldif')
        ldap_init_deploy_file = os.path.join(self.test_data_dir, 'create_test_data_deploy.ldif')

        shutil.copy(ldap_init_file, ldap_init_deploy_file)
        for line in fileinput.input(ldap_init_deploy_file, inplace=True):
            if 'sk: ' in line:
                secret_key = line[4:].rstrip()
                encrypted_secret_key = LdapSetup.__encrypt_secret_key(secret_key)
                line = f"sk: {encrypted_secret_key}\n"

            print(line, end='')

        cmd = "ldapadd -h %s -p %s -w %s -x -D %s -f %s" % (self.ldap_config['host'],
                self.ldap_config['port'], self.ldap_config['password'],
                self.ldap_config['login_dn'], ldap_init_deploy_file)
        obj = call(cmd, shell=True)
        os.remove(ldap_init_deploy_file)

    def ldap_delete_all(self):
        cleanup_records = ["ou=accesskeys,dc=s3,dc=seagate,dc=com",
                        "ou=accounts,dc=s3,dc=seagate,dc=com",
                        "ou=idp,dc=s3,dc=seagate,dc=com"]

        for entry in cleanup_records:
                cmd = "ldapdelete -r -h %s -p %s -w %s -x -D %s -r %s" % (self.ldap_config['host'],
                        self.ldap_config['port'], self.ldap_config['password'],
                        self.ldap_config['login_dn'], entry)
                obj = call(cmd, shell=True)

    @staticmethod
    def __encrypt_secret_key(secret_key):
        encrypt_cmd = f'java -jar /opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar -s {secret_key} -e aes'
        args = split(encrypt_cmd)
        completed_process = check_output(args)
        return completed_process.decode().rstrip()


class LdapInfo:
    ldap_info_prop = dict()
    ldap_info_initialize = 0

    def init():
        if LdapInfo.ldap_info_initialize == 0:
            ldap_info_file = os.path.join(os.path.dirname(__file__),'s3iamcli_test_config.yaml')
            with open(ldap_info_file, 'r') as f:
                ldap_info = yaml.safe_load(f)
                LdapInfo.ldap_info_prop['ldap_admin'] = ldap_info['ldapuser']
                LdapInfo.ldap_info_prop['ldap_admin_pwd'] = ldap_info['ldappasswd']
        LdapInfo.ldap_info_initialize = 1

    def get_ldap_admin_pwd():
        LdapInfo.init()
        return LdapInfo.ldap_info_prop['ldap_admin_pwd']

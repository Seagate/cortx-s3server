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

class LdapSetup:
    def __init__(self):
        self.test_data_dir = os.path.join(os.path.dirname(__file__), 'test_data')

        ldap_config_file = os.path.join(self.test_data_dir, 'ldap_config.yaml')
        with open(ldap_config_file, 'r') as f:
            self.ldap_config = yaml.safe_load(f)

    def ldap_init(self):
        ldap_init_file = os.path.join(self.test_data_dir, 'create_test_data.ldif')
        cmd = "ldapadd -h %s -p %s -w %s -x -D %s -f %s" % (self.ldap_config['host'],
                self.ldap_config['port'], self.ldap_config['password'],
                self.ldap_config['login_dn'], ldap_init_file)
        obj = call(cmd, shell=True)

    def ldap_delete_all(self):
        ldap_delete_file = os.path.join(self.test_data_dir, 'clean_test_data.ldif')
        f = open(ldap_delete_file)
        lines=f.readlines()
        ldap_delete_without_copyright_file = os.path.join(self.test_data_dir, 'clean_test_data_without_copyright.ldif')
        fp = open(ldap_delete_without_copyright_file , "w")
        fp.write(lines[18])
        fp.write(lines[19])
        fp.write(lines[20])
        f.close()
        fp.close()
        cmd = "ldapdelete -r -h %s -p %s -w %s -x -D %s -f %s" % (self.ldap_config['host'],
                   self.ldap_config['port'], self.ldap_config['password'],
                                   self.ldap_config['login_dn'], ldap_delete_without_copyright_file)
        obj = call(cmd, shell=True)
        os.remove(ldap_delete_without_copyright_file)

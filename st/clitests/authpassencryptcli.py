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
import sys
import time
from threading import Timer
import subprocess
from framework import S3PyCliTest
from framework import Config
from framework import logit

class EncryptCLITest(S3PyCliTest):
    encryptcli_cmd = "java -jar /opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar"

    def __init__(self, description):
        super(EncryptCLITest, self).__init__(description)


    def run(self):
        super(EncryptCLITest, self).run()

    def teardown(self):
        super(EncryptCLITest, self).teardown()

    def with_cli(self, cmd):
        super(EncryptCLITest, self).with_cli(cmd)

    def encrypt_passwd(self, passwd):
        cmd =  "%s -s %s" % (self.encryptcli_cmd,
                    passwd)
        self.with_cli(cmd)
        return self

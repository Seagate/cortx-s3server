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

#!/usr/bin/python3.6

import os
from framework import Config
from framework import S3PyCliTest
from authpassencryptcli import EncryptCLITest
from s3client_config import S3ClientConfig


# Run AuthPassEncryptCLI tests.

#Encrypt password tests
EncryptCLITest('Encrypt given password').encrypt_passwd("seagate").execute_test().command_is_successful()

#Negative Tests
EncryptCLITest('Encrypt Invalid Passwd (only space)').encrypt_passwd("\" \"").execute_test(negative_case=True).command_should_fail().command_error_should_have('Invalid Password Value.')
EncryptCLITest('Encrypt Invalid Passwd (with space)').encrypt_passwd("\"xyz test\"").execute_test(negative_case=True).command_should_fail().command_error_should_have('Invalid Password Value.')
EncryptCLITest('Encrypt Invalid Passwd (empty)').encrypt_passwd("\"\"").execute_test(negative_case=True).command_should_fail().command_error_should_have('Invalid Password Value')
EncryptCLITest('No password value').encrypt_passwd("").execute_test(negative_case=True).command_should_fail().command_response_should_have('Usage: java -jar AuthPassEncryptCLI-1.0-0.jar [options]')


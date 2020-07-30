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
from framework import S3PyCliTest

class S3RecoveryTest(S3PyCliTest):
  def __init__(self, description):
    file_path = os.path.dirname(os.path.realpath(__file__))
    self.cmd = file_path + r"/../../s3recovery/s3recovery/s3recovery"
    super(S3RecoveryTest, self).__init__(description)

  def s3recovery_help(self):
    helpcmd = self.cmd + " --help"
    self.with_cli(helpcmd)
    return self

  def s3recovery_dry_run(self):
    dry_run_cmd = self.cmd + " --dry_run"
    self.with_cli(dry_run_cmd)
    return self

  def s3recovery_recover(self):
    recover_cmd = self.cmd + " --recover"
    self.with_cli(recover_cmd)
    return self


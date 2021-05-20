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

import sys

from setupcmd import SetupCmd, S3PROVError

class PostInstallCmd(SetupCmd):
  """PostInstall Setup Cmd."""
  name = "post_install"

  def __init__(self, config: str = None):
    """Constructor."""
    try:
      super(PostInstallCmd, self).__init__(config)
    except Exception as e:
      raise S3PROVError(f'exception: {e}\n')

  def process(self):
    """Main processing function."""
    self.logger.info("Running validations..\n")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    self.validate_config_files(self.name)
    self.logger.info("Validations passed..\n")

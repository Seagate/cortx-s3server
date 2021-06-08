#!/usr/bin/env python3
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

from setupcmd import SetupCmd, S3PROVError
from merge import merge_configs

class PostUpgradeCmd(SetupCmd):
  """Post Upgrade Setup Cmd."""
  name = "postupgrade"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(PostUpgradeCmd, self).__init__(config)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing {self.name}")
    try:
      self.logger.info("validations started")
      self.phase_prereqs_validate(self.name)
      self.logger.info("validations completed")

      # merge_configs() is imported from the merge.py
      # Upgrade config files
      self.logger.info("merge configs started")
      merge_configs()
      self.logger.info("merge configs completed")

      # Remove temporary .old files from S3 temporary location
      self.logger.info("Remove sample.old files started")
      regex = "*.sample.old"
      self.DeleteFileOrDirWithRegex(self.s3_tmp_dir, regex)
      self.logger.info("Remove sample.old files completed")

      # Validating config files after upgrade
      self.logger.info("config file validations started")
      self.validate_config_files(self.name)
      self.logger.info("config file validations completed")
    except Exception as e:
      raise S3PROVError(f'process: {self.name} failed with exception: {e}')

